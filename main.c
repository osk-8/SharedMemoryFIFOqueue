#include <stdio.h>
#include <stdlib.h>

#include "fifo_queue.h"

int main()
{
    printf("Create queue\n");
    struct Queue *queue = createQueue(PATHNAME, PROJ_ID);

    enqueue(queue, (void *)10);
    enqueue(queue, (void *)20);
    enqueue(queue, (void *)30);
    enqueue(queue, (void *)40);
    enqueue(queue, (void *)"Test");

    printf("Dequeue value: %d\n", (int)dequeue(queue));
    printf("Dequeue value: %d\n", (int)dequeue(queue));
    printf("Dequeue value: %d\n", (int)dequeue(queue));
    printf("Dequeue value: %d\n", (int)dequeue(queue));
    printf("Dequeue value: %s\n", (char *)dequeue(queue));

    printf("Size of queue: %d - should be 0\n", queue->size);

    shmdt(queue);
    exit(EXIT_SUCCESS);
}
