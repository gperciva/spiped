#ifndef _PTHREAD_UTIL_H_
#define _PTHREAD_UTIL_H_

/*
 * pthread_create_blocking(thread, attr, start_routine, arg):
 * Run pthread_create and block until the ${start_routine} has
 * called a function to indicate that it has finished initialization.
 */
int pthread_create_blocking(pthread_t * restrict,
    const pthread_attr_t * restrict,
    void *(*)(void *, void *(void *), void *), void * restrict);

#endif /* !_PTHREAD_UTIL_H_ */
