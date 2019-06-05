#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

struct Queue *create_queue(const char *name, const int number_of_elements, const size_t element_size);
struct Queue *get_queue(const char *name);
void *get_data_segment(const char *name, const size_t size);
void enqueue(struct Queue *queue, void *item, size_t mem_size);
void *dequeue(struct Queue *queue);
void close_queue(const char *name);

//support
char *concat_strings(char *dest, const char *a, const char *b);

struct Queue
{
    struct
    {
        enum
        {
            first_segment,
            second_segment
        } seg_to_read;

        size_t data_seq_capacity;
        size_t element_size;
        int data_seg_front[2];
        int data_seg_rear[2];
        size_t data_seg_size[2];
    } header;

    // sem_t *sem[2];
    // void *data_seg[2];
};

struct Queue *create_queue(const char *name, const int number_of_elements, const size_t element_size)
{
    int shm_fd;
    struct Queue *queue;
    char *name_string = malloc(sizeof(char) * 100);

    shm_fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(struct Queue)) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    queue = mmap(0, sizeof(struct Queue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (queue == (struct Queue *)(-1))
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // queue->sem[0] = sem_open("producer", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, number_of_elements);
    // if (queue->sem[0] == SEM_FAILED)
    // {
    //     perror("sem_open");
    //     exit(EXIT_FAILURE);
    // }

    // queue->sem[1] = sem_open("consumer", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 0);
    // if (queue->sem[1] == SEM_FAILED)
    // {
    //     perror("sem_open");
    //     exit(EXIT_FAILURE);
    // }

    sem_open("producer", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, number_of_elements);
    sem_open("consumer", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 0);

    // queue->data_seg[0] = get_data_segment(concat_strings(name_string, "bucket1_", name), number_of_elements * element_size);
    // queue->data_seg[1] = get_data_segment(concat_strings(name_string, "bucket2_", name), number_of_elements * element_size);
    get_data_segment("seg1", number_of_elements * element_size);
    get_data_segment("seg1", number_of_elements * element_size);

    queue->header.data_seq_capacity = number_of_elements * element_size;
    queue->header.element_size = element_size;
    queue->header.seg_to_read = first_segment;
    queue->header.data_seg_front[0] = queue->header.data_seg_front[1] = 0;
    queue->header.data_seg_rear[0] = queue->header.data_seg_rear[1] = 0;
    queue->header.data_seg_size[0] = queue->header.data_seg_size[1] = 0;

    free(name_string);

    return queue;
}

struct Queue *get_queue(const char *name)
{
    int shm_fd;
    struct Queue *queue;
    char *name_string = malloc(sizeof(char) * 100);

    shm_fd = shm_open(name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(struct Queue)) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    queue = mmap(0, sizeof(struct Queue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (queue == (struct Queue *)(-1))
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    free(name_string);

    return queue;
}

void *get_data_segment(const char *name, const size_t size)
{
    int shm_fd;
    void *data;

    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, size) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    data = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == (void *)(-1))
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return data;
}

void enqueue(struct Queue *queue, void *item, size_t mem_size)
{
    sem_t *sem[2];
    sem[0] = sem_open("producer", 0);
    sem[1] = sem_open("consumer", 0);

    sem_wait(sem[0]);
    void *seg[2];
    seg[0] = get_data_segment("seg1", queue->header.data_seq_capacity);
    seg[1] = get_data_segment("seg2", queue->header.data_seq_capacity);

    int index = !queue->header.seg_to_read;
    printf("Write to segment: %d\n", index);

    memcpy(seg[index] + queue->header.data_seg_rear[index], item, mem_size);
    queue->header.data_seg_rear[index] = (queue->header.data_seg_rear[index] + queue->header.element_size) % queue->header.data_seq_capacity;
    queue->header.data_seg_size[index]++;

    sem_post(sem[1]);
}

void *dequeue(struct Queue *queue)
{
    sem_t *sem[2];
    sem[0] = sem_open("producer", 0);
    sem[1] = sem_open("consumer", 0);

    void *seg[2];
    seg[0] = get_data_segment("seg1", queue->header.data_seq_capacity);
    seg[1] = get_data_segment("seg2", queue->header.data_seq_capacity);

    void *ptr;

    if (!queue->header.data_seg_size[queue->header.seg_to_read])
    {
        sem_wait(sem[1]);
        queue->header.seg_to_read = !queue->header.seg_to_read;
    }
    else
    {
        sem_wait(sem[1]);
    }

    int index = queue->header.seg_to_read;
    printf("Read from segmen: %d\n", index);

    ptr = seg[index] + queue->header.data_seg_front[index];
    queue->header.data_seg_front[index] = (queue->header.data_seg_front[index] + queue->header.element_size) % queue->header.data_seq_capacity;
    queue->header.data_seg_size[index]--;

    sem_post(sem[0]);

    return ptr;
}

void close_queue(const char *name)
{
    char *name_string = malloc(sizeof(char) * 100);

    if (shm_unlink("seg1") == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink("seg2") == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink("producer") == -1)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink("consumer") == -1)
    {
        perror("sem_unlink  ");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink(name) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    free(name_string);
}

char *concat_strings(char *dest, const char *a, const char *b)
{
    strcpy(dest, a);
    strcat(dest, b);

    return dest;
}