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

#define BUCKET_SIZE 1024
#define MEM_CHUNK 24

void *get_data_segment(const char *name, const size_t size);
char *concat_strings(char *dest, const char *a, const char *b);

struct Queue
{
    struct
    {
        size_t bucket_capacity;
        int bucket_to_read; //TO DO: should be enum
        int data_seg_front[2];
        int data_seg_rear[2];
        size_t data_seg_size[2];
    } header;

    sem_t *sem[2];
    void *data_seg[2];
};

struct Queue *create_queue(const char *name)
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

    queue->sem[0] = sem_open(concat_strings(name_string, "sem1_", name), O_CREAT, 0666, 1);
    if (queue->sem[0] == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    queue->sem[1] = sem_open(concat_strings(name_string, "sem2_", name), O_CREAT, 0666, 1);
    if (queue->sem[1] == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    queue->data_seg[0] = get_data_segment(concat_strings(name_string, "bucket1_", name), BUCKET_SIZE);
    queue->data_seg[1] = get_data_segment(concat_strings(name_string, "bucket2_", name), BUCKET_SIZE);

    queue->header.bucket_capacity = BUCKET_SIZE;
    queue->header.bucket_to_read = 1;
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

    queue->sem[0] = sem_open(concat_strings(name_string, "sem1_", name), 0);
    if (queue->sem[0] == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    queue->sem[1] = sem_open(concat_strings(name_string, "sem2_", name), 0);
    if (queue->sem[1] == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    queue->data_seg[0] = get_data_segment(concat_strings(name_string, "bucket1_", name), BUCKET_SIZE);
    queue->data_seg[1] = get_data_segment(concat_strings(name_string, "bucket2_", name), BUCKET_SIZE);

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
    if (queue->header.bucket_to_read == 1)
    {
        if (queue->header.data_seg_size[1] && (queue->header.data_seg_front[1] == queue->header.data_seg_rear[1]))
        {
            //SEMAPHORE IN ACTION  [is full] -> [wait for swap in dequeue]
        }
    }
    else
    {
        if (queue->header.data_seg_size[0] && (queue->header.data_seg_front[0] == queue->header.data_seg_rear[0]))
        {
            //SEMAPHORE IN ACTION
        }
    }

    if (queue->header.bucket_to_read == 1)
    {
        sem_wait(queue->sem[1]);
        memcpy(queue->data_seg[1] + queue->header.data_seg_rear[1], item, mem_size);
        queue->header.data_seg_rear[1] = (queue->header.data_seg_rear[1] + MEM_CHUNK) % queue->header.bucket_capacity;
        queue->header.data_seg_size[1]++;
        sem_post(queue->sem[1]);
    }
    else
    {
        sem_wait(queue->sem[0]);
        memcpy(queue->data_seg[0] + queue->header.data_seg_rear[0], item, mem_size);
        queue->header.data_seg_rear[0] = (queue->header.data_seg_rear[0] + MEM_CHUNK) % queue->header.bucket_capacity;
        queue->header.data_seg_size[0]++;
        sem_post(queue->sem[0]);
    }
}

void *dequeue(struct Queue *queue)
{
    void *ptr;

    if (!queue->header.data_seg_size[0] && !queue->header.data_seg_size[1])
    {
        printf("Queue is empty!");
        exit(EXIT_FAILURE);
    }

    if (queue->header.bucket_to_read == 1 && !queue->header.data_seg_size[0])
        queue->header.bucket_to_read = 2;
    else if (queue->header.bucket_to_read == 2 && !queue->header.data_seg_size[1])
        queue->header.bucket_to_read = 1;

    if (queue->header.bucket_to_read == 1)
    {
        sem_wait(queue->sem[0]);
        ptr = queue->data_seg[0] + queue->header.data_seg_front[0];
        queue->header.data_seg_front[0] = (queue->header.data_seg_front[0] + MEM_CHUNK) % queue->header.bucket_capacity;
        queue->header.data_seg_size[0]--;
        sem_post(queue->sem[0]);
    }
    else
    {
        sem_wait(queue->sem[1]);
        ptr = queue->data_seg[1] + queue->header.data_seg_front[1];
        queue->header.data_seg_front[1] = (queue->header.data_seg_front[1] + MEM_CHUNK) % queue->header.bucket_capacity;
        queue->header.data_seg_size[1]--;
        sem_post(queue->sem[1]);
    }

    return ptr;
}

void rm_queue(const char *name)
{
    char *name_string = malloc(sizeof(char) * 100);

    if (shm_unlink(concat_strings(name_string, "bucket1_", name)) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink(concat_strings(name_string, "bucket2_", name)) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(concat_strings(name_string, "sem1_", name)) == -1)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(concat_strings(name_string, "sem2_", name)) == -1)
    {
        perror("sem_unlink");
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