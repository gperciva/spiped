#ifndef _PUSHBITS_H_
#define _PUSHBITS_H_

#include <sys/types.h>

/**
 * pushbits(in, out, thr):
 * Create a thread which copies data from ${in} to ${out} and
 * store the thread ID in ${thr}.  Wait until ${thr} has started.
 * If ${out} is a socket, disable writing to it after the thread
 * exits.
 */
int pushbits(int, int, pthread_t *);

#endif /* !_PUSHBITS_H_ */
