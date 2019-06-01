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

#define FIRST_SEM_NAME "first_sem"
#define SECOND_SEM_NAME "second_sem"
#define FIRST_BUCKET_NAME "first_bucket"
#define SECOND_BUCKET_NAME "second_bucket" //TO DO: use concat
#define BUCKET_SIZE 1024
#define MEM_CHUNK 24

void *get_data_segment(const char *name, const size_t size);

struct Queue
{
    struct
    {
        size_t bucket_capacity;
        int bucket_to_read; //TO DO: should be enum
        int bucket1_front, bucket2_front;
        int bucket1_rear, bucket2_rear;
        size_t bucket1_size, bucket2_size;
    } header;

    sem_t *sem1;
    sem_t *sem2;

    void *bucket1;
    void *bucket2;
};

struct Queue *create_queue(const char *name)
{
    int shm_fd;
    struct Queue *queue;

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

    queue->sem1 = sem_open(FIRST_SEM_NAME, O_CREAT, 0666, 1);
    if (queue->sem1 == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    queue->sem2 = sem_open(SECOND_SEM_NAME, O_CREAT, 0666, 1);
    if (queue->sem2 == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    queue->bucket1 = get_data_segment(FIRST_BUCKET_NAME, BUCKET_SIZE);
    queue->bucket2 = get_data_segment(SECOND_BUCKET_NAME, BUCKET_SIZE);

    queue->header.bucket_capacity = BUCKET_SIZE;
    queue->header.bucket_to_read = 1;
    queue->header.bucket1_front = queue->header.bucket2_front = 0;
    queue->header.bucket1_rear = queue->header.bucket2_rear = 0;
    queue->header.bucket1_size = queue->header.bucket2_size = 0;

    return queue;
}

struct Queue *get_queue(const char *name)
{
    int shm_fd;
    struct Queue *queue;

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

    queue->sem1 = sem_open(FIRST_SEM_NAME, 0);
    if (queue->sem1 == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    queue->sem2 = sem_open(SECOND_SEM_NAME, 0);
    if (queue->sem2 == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    queue->bucket1 = get_data_segment(FIRST_BUCKET_NAME, BUCKET_SIZE);
    queue->bucket2 = get_data_segment(SECOND_BUCKET_NAME, BUCKET_SIZE);

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
        if (queue->header.bucket2_size && (queue->header.bucket2_front == queue->header.bucket2_rear))
        {
            //SEMAPHORE IN ACTION  [is full] -> [wait for swap in dequeue]
        }
    }
    else
    {
        if (queue->header.bucket1_size && (queue->header.bucket1_front == queue->header.bucket1_rear))
        {
            //SEMAPHORE IN ACTION
        }
    }

    if (queue->header.bucket_to_read == 1)
    {
        sem_wait(queue->sem2);
        memcpy(queue->bucket2 + queue->header.bucket2_rear, item, mem_size);
        queue->header.bucket2_rear = (queue->header.bucket2_rear + MEM_CHUNK) % queue->header.bucket_capacity;
        queue->header.bucket2_size++;
        sem_post(queue->sem2);
    }
    else
    {
        sem_wait(queue->sem1);
        memcpy(queue->bucket1 + queue->header.bucket1_rear, item, mem_size);
        queue->header.bucket1_rear = (queue->header.bucket1_rear + MEM_CHUNK) % queue->header.bucket_capacity;
        queue->header.bucket1_size++;
        sem_post(queue->sem1);
    }
}

void *dequeue(struct Queue *queue)
{
    void *ptr;

    if (!queue->header.bucket1_size && !queue->header.bucket2_size)
    {
        printf("Queue is empty!");
        exit(EXIT_FAILURE);
    }

    if (queue->header.bucket_to_read == 1 && !queue->header.bucket1_size)
        queue->header.bucket_to_read = 2;
    else if (queue->header.bucket_to_read == 2 && !queue->header.bucket2_size)
        queue->header.bucket_to_read = 1;

    if (queue->header.bucket_to_read == 1)
    {
        sem_wait(queue->sem1);
        ptr = queue->bucket1 + queue->header.bucket1_front;
        queue->header.bucket1_front = (queue->header.bucket1_front + MEM_CHUNK) % queue->header.bucket_capacity;
        queue->header.bucket1_size--;
        sem_post(queue->sem1);
    }
    else
    {
        sem_wait(queue->sem2);
        ptr = queue->bucket2 + queue->header.bucket2_front;
        queue->header.bucket2_front = (queue->header.bucket2_front + MEM_CHUNK) % queue->header.bucket_capacity;
        queue->header.bucket2_size--;
        sem_post(queue->sem2);
    }

    return ptr;
}

void rm_queue(const char *name)
{
    if (shm_unlink(FIRST_BUCKET_NAME) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink(SECOND_BUCKET_NAME) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(FIRST_SEM_NAME) == -1)
    {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(SECOND_SEM_NAME) == -1)
    {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink(name) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }
}