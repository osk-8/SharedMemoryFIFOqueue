#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct Queue *create_queue(unsigned int proj_id, const int number_of_elements, const size_t element_size);
struct Queue *get_queue(unsigned int proj_id);
void enqueue(struct Queue *queue, void *item, size_t mem_size);
void *dequeue(struct Queue *queue);
void close_queue(struct Queue *queue);

struct Queue
{
    struct
    {
        int shm_id;
        size_t data_seg_capacity;
        size_t element_size;
        int data_seg_front[2];
        int data_seg_rear[2];
        size_t data_seg_size[2];

        enum
        {
            first_segment,
            second_segment
        } seg_to_read;
    } header;

    int sem_id;
    int seg_id[2];
};
