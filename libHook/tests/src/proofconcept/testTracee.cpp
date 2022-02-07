#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifndef   THREADS
#define   THREADS  2
#endif

volatile sig_atomic_t   done = 0;

void catch_done(int signum)
{
    done = signum;
}

int install_done(const int signum)
{
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_handler = catch_done;
    act.sa_flags = 0;
    if (sigaction(signum, &act, NULL))
        return errno;
    else
        return 0;
}

void *worker(void *data)
{
    volatile unsigned long *const counter = data;

    while (!done)
        __sync_add_and_fetch(counter, 1UL);

    return (void *)(unsigned long)__sync_or_and_fetch(counter, 0UL);
}

int main(void)
{
    unsigned long   counter = 0UL;
    pthread_t       thread[THREADS];
    pthread_attr_t  attrs;
    size_t          i;

    if (install_done(SIGHUP) ||
        install_done(SIGTERM) ||
        install_done(SIGUSR1)) {
        fprintf(stderr, "Worker: Cannot install signal handlers: %s.\n", strerror(errno));
        return 1;
    }

    pthread_attr_init(&attrs);
    pthread_attr_setstacksize(&attrs, 65536);
    for (i = 0; i < THREADS; i++)
        if (pthread_create(&thread[i], &attrs, worker, &counter)) {
            done = 1;
            fprintf(stderr, "Worker: Cannot create thread: %s.\n", strerror(errno));
            return 1;
        }
    pthread_attr_destroy(&attrs);

    /* Let the original thread also do the worker dance. */
    worker(&counter);

    for (i = 0; i < THREADS; i++)
        pthread_join(thread[i], NULL);

    return 0;
}
