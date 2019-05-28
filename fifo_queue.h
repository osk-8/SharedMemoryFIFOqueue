#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define QUEUE_CAPACITY 100

struct Queue
{
    struct
    {
        size_t bucket_capacity;
        int bucket_to_read;
        int first_bucket_front;
        int second_bucket_front;
        int first_bucket_rear;
        int second_bucket_rear;
        int first_bucket_size;
        int second_bucket_size;
    } header;

    int first_bucket[QUEUE_CAPACITY];
    int second_bucket[QUEUE_CAPACITY];
};

struct Queue *createQueue(const char *name)
{
    int shm_fd;
    struct Queue *queue;

    if ((shm_fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0666)) == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(struct Queue)) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    if ((queue = mmap(0, sizeof(struct Queue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (struct Queue *)(-1))
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    queue->header.bucket_capacity = QUEUE_CAPACITY;
    queue->header.bucket_to_read = 1;
    queue->header.first_bucket_front = 0;
    queue->header.second_bucket_front = 0;
    queue->header.first_bucket_rear = 0;
    queue->header.second_bucket_rear = 0;
    queue->header.first_bucket_size = 0;
    queue->header.second_bucket_size = 0;

    return queue;
}

struct Queue *getQueue(const char *name)
{
    int shm_fd;
    struct Queue *queue;

    if ((shm_fd = shm_open(name, O_RDWR, 0666)) == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(struct Queue)) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    if ((queue = mmap(0, sizeof(struct Queue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (struct Queue *)(-1))
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return queue;
}

void enqueue(struct Queue *queue, int item)
{
    if (queue->header.bucket_to_read == 1)
    {
        queue->second_bucket[queue->header.second_bucket_rear] = item;
        queue->header.second_bucket_rear = (queue->header.second_bucket_rear + 1) % queue->header.bucket_capacity;
        queue->header.second_bucket_size++;
    }
    else
    {
        queue->first_bucket[queue->header.first_bucket_rear] = item;
        queue->header.first_bucket_rear = (queue->header.first_bucket_rear + 1) % queue->header.bucket_capacity;
        queue->header.first_bucket_size++;
    }
}

int dequeue(struct Queue *queue)
{
    int item;

    if (!queue->header.first_bucket_size && !queue->header.second_bucket_size)
    {
        printf("Queue is empty!");
        exit(EXIT_FAILURE);
    }

    if (queue->header.bucket_to_read == 1 && !queue->header.first_bucket_size)
        queue->header.bucket_to_read = 2;
    else if (queue->header.bucket_to_read == 2 && !queue->header.second_bucket_size)
        queue->header.bucket_to_read = 1;

    if (queue->header.bucket_to_read == 1)
    {
        item = queue->first_bucket[queue->header.first_bucket_front];
        queue->header.first_bucket_front++;
    }
    else
    {
        item = queue->second_bucket[queue->header.second_bucket_front];
        queue->header.second_bucket_front++;
    }

    return item;
}

void rmFifoQueue(const char *name)
{
    if (shm_unlink(name) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }
}