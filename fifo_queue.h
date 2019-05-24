#pragma once

#include <limits.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PATHNAME "./fifo_queue.c"
#define PROJ_ID "O"
#define QUEUE_CAPACITY 100

struct Queue
{
    size_t front, rear, size;
    void *data[QUEUE_CAPACITY];
};

struct Queue *createQueue(const char *pathname, const char *proj_id)
{
    key_t key;
    int shmid;
    key = ftok(pathname, proj_id);
    shmid = shmget(key, sizeof(struct Queue), 0644 | IPC_CREAT);

    struct Queue *queue = shmat(shmid, (void *)0, 0);
    queue->front = 0;
    queue->rear = QUEUE_CAPACITY - 1;
    queue->size = 0;

    return queue;
}

struct Queue *getQueue(const char *pathname, const char *proj_id)
{
    key_t key;
    int shmid;
    key = ftok(pathname, proj_id);
    shmid = shmget(key, sizeof(struct Queue), 0644);

    struct Queue *queue = shmat(shmid, (void *)0, 0);
    return queue;
}

int isFull(struct Queue *queue)
{
    return (queue->size == QUEUE_CAPACITY);
}

int isEmpty(struct Queue *queue)
{
    return (queue->size == 0);
}

void enqueue(struct Queue *queue, void *item)
{
    if (isFull(queue))
        return;

    queue->rear = (queue->rear + 1) % QUEUE_CAPACITY;
    queue->data[queue->rear] = item;
    queue->size += 1;
}

void *dequeue(struct Queue *queue)
{
    if (isEmpty(queue))
        return;

    void *item = queue->data[queue->front];
    queue->front = (queue->front + 1) % QUEUE_CAPACITY;
    queue->size -= 1;
    return item;
}
