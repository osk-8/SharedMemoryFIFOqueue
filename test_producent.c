#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fifo_queue.h"

int main()
{
    struct Queue *queue = create_queue(77, 10, 24);

    while (1)
    {
        getchar();
        enqueue(queue, "TEST", sizeof(char) * 24);
    }

    exit(EXIT_SUCCESS);
}
