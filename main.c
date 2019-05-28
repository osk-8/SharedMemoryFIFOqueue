#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fifo_queue.h"

int main()
{
    struct Queue *queue = createQueue("my_queue");

    enqueue(queue, 0);
    enqueue(queue, 100);
    enqueue(queue, 250);
    enqueue(queue, 500);

    exit(EXIT_SUCCESS);
}

// #define SEM_NAME "/mysem"
//     sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0777, 1);
//     sem_wait(sem);
//     sem_post(sem);