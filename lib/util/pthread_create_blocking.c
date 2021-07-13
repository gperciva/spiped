#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "warnp.h"

#include "pthread_create_blocking.h"

struct util_cookie {
	void *(*start_routine)(void *, void *(void *), void *);
	void * cookie;

	/* The below are only valid during thread startup. */
	int * initialized_p;
	pthread_mutex_t * mutex_p;
	pthread_cond_t * cond_p;
};

/* Will be called by the client code. */
static void *
initialized_func(void * cookie)
{
	struct util_cookie * U = cookie;
	int rc;

	/* Indicate that the client has finished initializing. */
	if ((rc = pthread_mutex_lock(U->mutex_p)) != 0) {
		warn0("pthread_mutex_lock: %s", strerror(rc));
		exit(1);
	}
	*U->initialized_p = 1;
	if ((rc = pthread_cond_signal(U->cond_p)) != 0) {
		warn0("pthread_cond_signal: %s", strerror(rc));
		exit(1);
	}
	if ((rc = pthread_mutex_unlock(U->mutex_p)) != 0) {
		warn0("pthread_mutex_unlock: %s", strerror(rc));
		exit(1);
	}

	return (NULL);
}

static void *
wrapped_start(void * cookie)
{
	struct util_cookie * U = cookie;

	/* Call the supplied function with the extra function and cookie. */
	return (U->start_routine(U->cookie, initialized_func, U));
}

/**
 * pthread_create_blocking(thread, attr, start_routine, arg):
 * Run pthread_create and block until the ${start_routine} has started.
 * The ${start_routine} must be
 *     void * start_routine(void * cookie, void * init(void*),
 *         void * init_cookie)
 * When ${start_routine} is ready for the main thread to continue, it must
 * call init(init_cookie).
 */
int
pthread_create_blocking(pthread_t * restrict thread,
    const pthread_attr_t * restrict attr,
    void *(*start_routine)(void *, void *(void *), void *),
    void * restrict arg)
{
	struct util_cookie * U;
	int rc;
	int initialized;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	/* Initialize thread startup-related variables. */
	initialized = 0;
	if ((rc = pthread_mutex_init(&mutex, NULL)) != 0) {
		warn0("pthread_mutex_init: %s", strerror(rc));
		goto err0;
	}
	if ((rc = pthread_cond_init(&cond, NULL)) != 0) {
		warnp("pthread_cond_init: %s", strerror(rc));
		goto err1;
	}

	/* Allocate our cookie. */
	if ((U = malloc(sizeof(struct util_cookie))) == NULL) {
		warnp("malloc");
		goto err2;
	}
	U->cookie = arg;
	U->start_routine = start_routine;
	U->initialized_p = &initialized;
	U->mutex_p = &mutex;
	U->cond_p = &cond;

	/* Create the thread. */
	if ((rc = pthread_create(thread, attr, wrapped_start, U)) != 0) {
		warn0("pthread_create: %s", strerror(rc));
		goto err3;
	}

	/* Wait for the thread to have started. */
	if ((rc = pthread_mutex_lock(&mutex)) != 0) {
		warn0("pthread_mutex_lock: %s", strerror(rc));

		/* Don't try to clean up if there's an error here. */
		exit(1);
	}
	while (!initialized) {
		if ((rc = pthread_cond_wait(&cond, &mutex)) != 0) {
			warn0("pthread_cond_signal: %s", strerror(rc));
			exit(1);
		}
	}
	if (pthread_mutex_unlock(&mutex)) {
		warn0("pthread_mutex_unlock: %s", strerror(rc));
		exit(1);
	}

	/* Clean up startup-related variables. */
	if ((rc = pthread_cond_destroy(&cond)) != 0) {
		warn0("pthread_cond_destroy: %s", strerror(rc));
		goto err2;
	}
	if ((rc = pthread_mutex_destroy(&mutex)) != 0) {
		warn0("pthread_mutex_destroy: %s", strerror(rc));
		goto err1;
	}
	free(U);

	/* Success! */
	return (0);

err3:
	free(U);
err2:
	if ((rc = pthread_cond_destroy(&cond)) != 0)
		warn0("pthread_cond_destroy: %s", strerror(rc));
err1:
	if ((rc = pthread_mutex_destroy(&mutex)) != 0)
		warn0("pthread_mutex_destroy: %s", strerror(rc));
err0:
	/* Failure! */
	return (-1);
}
