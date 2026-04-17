/* semaphore.h -- Semaphore stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SEMAPHORE_H
#define _BAREMETAL_SEMAPHORE_H

typedef struct {
    volatile int __val;
} sem_t;

#define SEM_FAILED ((sem_t *)0)

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);
sem_t *sem_open(const char *name, int oflag, ...);
int sem_close(sem_t *sem);
int sem_unlink(const char *name);

#endif /* _BAREMETAL_SEMAPHORE_H */
