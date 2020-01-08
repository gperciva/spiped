#ifndef _PUSHBITS_H_
#define _PUSHBITS_H_

#include <sys/types.h>

/**
 * pushbits(in, out, thr):
 * Create a thread which copies data from ${in} to ${out} and
 * store the thread ID in ${thr}.  Wait until the thread has started.
 */
int pushbits(int, int, pthread_t *);

#endif /* !_PUSHBITS_H_ */
