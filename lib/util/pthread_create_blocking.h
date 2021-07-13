#ifndef _PTHREAD_CREATE_BLOCKING_H_
#define _PTHREAD_CREATE_BLOCKING_H_

/**
 * pthread_create_blocking(thread, attr, start_routine, arg):
 * Run pthread_create and block until the ${start_routine} has started.
 * The ${start_routine} must be
 *     void * start_routine(void * cookie, void * init(void*),
 *         void * init_cookie)
 * When ${start_routine} is ready for the main thread to continue, it must
 * call init(init_cookie).
 */
int pthread_create_blocking(pthread_t * restrict,
    const pthread_attr_t * restrict,
    void *(*)(void *, void *(void *), void *), void * restrict);

#endif /* !_PTHREAD_CREATE_BLOCKING_H_ */
