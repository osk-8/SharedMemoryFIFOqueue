#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fifo_queue.h"

int main()
{
    struct Queue *queue = create_queue("my_queue", 10, 24);
    // struct Queue *queue = get_queue("my_queue");

    // while (1)
    // {
    //     getchar();
    //     getchar();
    //     enqueue(queue, "TEST10", sizeof(char) * 10);
    // }

    enqueue(queue, "TEST70", sizeof(char) * 6);
    enqueue(queue, "TEST77", sizeof(char) * 6);
    enqueue(queue, "TEST98", sizeof(char) * 6);
    enqueue(queue, "TEST98", sizeof(char) * 6);
    enqueue(queue, "WOW", sizeof(char) * 6);

    exit(EXIT_SUCCESS);
}
