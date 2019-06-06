#include "fifo_queue.h"

struct Queue *create_queue(unsigned int proj_id, const int number_of_elements, const size_t element_size)
{
    key_t key;
    int shmid;
    struct Queue *ptr_queue;

    key = ftok("/home/osk/Projects/SharedMemoryFIFOqueue/fifo_queue.h", proj_id);
    if (key == -1)
    {
        perror("queue ftok");
    }
    shmid = shmget(proj_id, sizeof(struct Queue), 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("queue shmid");
    }
    ptr_queue = (struct Queue *)shmat(shmid, (void *)0, 0);
    if (ptr_queue == (struct Queue *)(-1))
    {
        perror("queue shmid");
        exit(EXIT_FAILURE);
    }

    key = ftok("/home/osk/Projects/SharedMemoryFIFOqueue/fifo_queue.h", proj_id + 1);
    if (key == -1)
    {
        perror("seg1 ftok");
    }
    ptr_queue->seg_id[0] = shmget(proj_id + 1, number_of_elements * element_size, 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("seg1 shmid");
    }

    key = ftok("/home/osk/Projects/SharedMemoryFIFOqueue/fifo_queue.h", proj_id + 2);
    if (key == -1)
    {
        perror("seg2 ftok");
    }
    ptr_queue->seg_id[1] = shmget(proj_id + 2, number_of_elements * element_size, 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("seg2 shmid");
    }

    key = ftok("/home/osk/Projects/SharedMemoryFIFOqueue/fifo_queue.h", proj_id + 3);
    if (key == -1)
    {
        perror("sem ftok");
    }

    ptr_queue->sem_id = semget(proj_id + 3, 2, 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("sem shmid");
    }

    int status = semctl(ptr_queue->sem_id, 0, SETVAL, number_of_elements);
    if (status == -1)
    {
        perror("sem1 semctl");
    }

    status = semctl(ptr_queue->sem_id, 1, SETVAL, 0);
    if (status == -1)
    {
        perror("sem2 semctl");
    }

    ptr_queue->header.shm_id = shmid;
    ptr_queue->header.data_seg_capacity = number_of_elements * element_size;
    ptr_queue->header.element_size = element_size;
    ptr_queue->header.seg_to_read = first_segment;
    ptr_queue->header.data_seg_front[0] = ptr_queue->header.data_seg_front[1] = 0;
    ptr_queue->header.data_seg_rear[0] = ptr_queue->header.data_seg_rear[1] = 0;
    ptr_queue->header.data_seg_size[0] = ptr_queue->header.data_seg_size[1] = 0;

    return ptr_queue;
}

struct Queue *get_queue(unsigned int proj_id)
{
    key_t key;
    int shmid;
    struct Queue *ptr_queue;

    key = ftok("/home/osk/Projects/SharedMemoryFIFOqueue/fifo_queue.h", proj_id);
    if (key == -1)
    {
        perror("queue ftok");
    }
    shmid = shmget(proj_id, sizeof(struct Queue), 0);
    if (shmid == 0)
    {
        perror("queue shmget");
    }
    ptr_queue = shmat(shmid, (void *)0, 0);
    if (ptr_queue == (struct Queue *)(-1))
    {
        perror("queue shmat");
        exit(EXIT_FAILURE);
    }

    return ptr_queue;
}

void enqueue(struct Queue *queue, void *item, size_t mem_size)
{
    struct sembuf operation[2];
    operation[0].sem_num = 0;
    operation[0].sem_op = -1;
    operation[0].sem_flg = 0;
    operation[1].sem_num = 1;
    operation[1].sem_op = 1;
    operation[1].sem_flg = 0;

    int status = semop(queue->sem_id, &operation[0], 1);
    if (status == -1)
    {
        perror("can not semctl");
    }

    int index = !queue->header.seg_to_read;

    void *data_seg = shmat(queue->seg_id[index], (void *)0, 0);
    if (data_seg == (void *)(-1))
    {
        perror("seg shmat");
        exit(EXIT_FAILURE);
    }

    memcpy(data_seg + queue->header.data_seg_rear[index], item, mem_size);
    queue->header.data_seg_rear[index] = (queue->header.data_seg_rear[index] + queue->header.element_size) % queue->header.data_seg_capacity;
    queue->header.data_seg_size[index]++;

    status = semop(queue->sem_id, &operation[1], 1);
    if (status == -1)
    {
        perror("can not semctl");
    }
}

void *dequeue(struct Queue *queue)
{
    int status;
    struct sembuf operation[2];
    operation[0].sem_num = 0;
    operation[0].sem_op = 1;
    operation[0].sem_flg = 0;
    operation[1].sem_num = 1;
    operation[1].sem_op = -1;
    operation[1].sem_flg = 0;

    if (!queue->header.data_seg_size[queue->header.seg_to_read])
    {
        status = semop(queue->sem_id, &operation[1], 1);
        queue->header.seg_to_read = !queue->header.seg_to_read;
    }
    else
    {
        status = semop(queue->sem_id, &operation[1], 1);
    }

    if (status == -1)
    {
        perror("can not semctl");
    }

    int index = queue->header.seg_to_read;

    void *data_seg = shmat(queue->seg_id[index], (void *)0, 0);
    if (data_seg == (void *)(-1))
    {
        perror("seg shmat");
        exit(EXIT_FAILURE);
    }

    void *ptr = data_seg + queue->header.data_seg_front[index];
    queue->header.data_seg_front[index] = (queue->header.data_seg_front[index] + queue->header.element_size) % queue->header.data_seg_capacity;
    queue->header.data_seg_size[index]--;

    status = semop(queue->sem_id, &operation[0], 1);
    if (status == -1)
    {
        perror("can not semctl");
    }

    return ptr;
}

void close_queue(struct Queue *queue)
{
    if (shmctl(queue->seg_id[0], IPC_RMID, NULL) == -1)
    {
        perror("seg1) semctl");
    }

    if (shmctl(queue->seg_id[1], IPC_RMID, NULL) == -1)
    {
        perror("seg2) semctl");
    }

    if (semctl(queue->sem_id, 0, IPC_RMID) == -1)
    {
        perror("semctl");
    }

    if (shmctl(queue->header.shm_id, IPC_RMID, NULL) == -1)
    {
        perror("queue) semctl");
        exit(EXIT_FAILURE);
    }
}
