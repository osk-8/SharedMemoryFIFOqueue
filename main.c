#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fifo_queue.h"

int main()
{
    struct Queue *queue = create_queue("my_queue");

    enqueue(queue, "TEST10", sizeof(char) * 6);
    enqueue(queue, "TEST20", sizeof(char) * 6);
    enqueue(queue, "TEST70", sizeof(char) * 6);
    enqueue(queue, "TEST77", sizeof(char) * 6);
    enqueue(queue, "TEST98", sizeof(char) * 6);

    exit(EXIT_SUCCESS);
}
