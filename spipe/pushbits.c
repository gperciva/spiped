#include <sys/socket.h>

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "noeintr.h"
#include "pthread_util.h"
#include "warnp.h"

#include "pushbits.h"

struct push {
	uint8_t buf[BUFSIZ];
	int in;
	int out;
};

static void
workthread_cleanup(void * cookie)
{
	struct push * P = cookie;

	/* Close the descriptor (we either hit EOF, or received a cancel). */
	close(P->in);

	/*
	 * Try to shut down the descriptor we're writing to.  Ignore ENOTSOCK,
	 * since it might, indeed, not be a socket.
	 */
	if (shutdown(P->out, SHUT_WR)) {
		if (errno != ENOTSOCK) {
			warnp("Error shutting down socket");
			exit(1);
		}
	}

	/* Free our parameters. */
	free(P);
}

/* Bit-pushing thread. */
static void *
workthread(void * cookie, void * initialized(void *), void * initialized_cookie)
{
	struct push * P = cookie;
	ssize_t readlen;

	/* Set up cleanup function. */
	pthread_cleanup_push(workthread_cleanup, P);

	/* Notify that we've started. */
	initialized(initialized_cookie);

	/* Infinite loop unless we hit EOF or an error. */
	do {
		/* Read data and die on error. */
		if ((readlen = read(P->in, P->buf, BUFSIZ)) == -1) {
			if (errno == EINTR)
				continue;
			warnp("Error reading");
			exit(1);
		}

		/* If we hit EOF, exit the loop. */
		if (readlen == 0)
			break;

		/* Write the data back out. */
		if (noeintr_write(P->out, &P->buf, (size_t)readlen)
		    != readlen) {
			warnp("Error writing");
			exit(1);
		}
	} while (1);

	/* Clean up. */
	pthread_cleanup_pop(1);

	/* We're done. */
	return (NULL);
}

/**
 * pushbits(in, out, thr):
 * Create a thread which copies data from ${in} to ${out} and
 * store the thread ID in ${thr}.  Wait until the thread has started.
 */
int
pushbits(int in, int out, pthread_t * thr)
{
	struct push * P;
	int rc;

	/* Allocate structure. */
	if ((P = malloc(sizeof(struct push))) == NULL)
		goto err0;
	P->in = in;
	P->out = out;

	/* Create thread. */
	if ((rc = pthread_create_blocking(thr, NULL, workthread, P)) != 0) {
		warn0("pthread_create: %s", strerror(rc));
		goto err1;
	}

	/* Success! */
	return (0);

err1:
	free(P);
err0:
	/* Failure! */
	return (-1);
}
