#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#include "../fifo_queue.h"

int main()
{
    struct Queue *queue = get_queue(77);

    printf("Dequeue: %s\n", dequeue(queue));
    printf("Dequeue: %s\n", dequeue(queue));
    printf("Dequeue: %s\n", dequeue(queue));
    printf("Dequeue: %s\n", dequeue(queue));
    printf("Dequeue: %s\n", dequeue(queue));
    printf("Dequeue: %s\n", dequeue(queue));

    close_queue(queue);
}