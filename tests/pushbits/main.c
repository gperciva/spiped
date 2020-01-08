#include <sys/socket.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "getopt.h"
#include "noeintr.h"
#include "parsenum.h"
#include "warnp.h"

#include "pushbits.h"

static int
basic_copy(const char * filename_in, const char * filename_out)
{
	pthread_t thread;
	int in, out;
	int rc;

	/* Open input and output. */
	if ((in = open(filename_in, O_RDONLY)) == -1) {
		warnp("open(%s)", filename_in);
		goto err0;
	}
	if ((out = open(filename_out, O_WRONLY | O_CREAT | O_TRUNC,
	    S_IRUSR | S_IWUSR)) == -1) {
		warnp("open(%s)", filename_out);
		goto err1;
	}

	/* Start copying. */
	if (pushbits(in, out, &thread)) {
		warnp("pushbits");
		goto err2;
	}

	/* Wait for thread to finish. */
	if ((rc = pthread_join(thread, NULL)) != 0) {
		warn0("pthread_join: %s", strerror(rc));
		goto err2;
	}

	/* Clean up. */
	close(out);
	close(in);

	/* Success! */
	return (0);

err2:
	close(out);
err1:
	close(in);
err0:
	/* Failure! */
	return (-1);
}

static int
echo_stdin_stdout(void)
{
	pthread_t thread;
	int rc;

	/* Start echoing. */
	if (pushbits(STDIN_FILENO, STDOUT_FILENO, &thread)) {
		warnp("pushbits");
		goto err0;
	}

	/* Wait a short while, then cancel the thread. */
	sleep(1);
	if ((rc = pthread_cancel(thread)) != 0) {
		warn0("pthread_cancel: %s", strerror(rc));
		goto err0;
	}

	/* Wait for thread to finish. */
	if ((rc = pthread_join(thread, NULL)) != 0) {
		warn0("pthread_join: %s", strerror(rc));
		goto err0;
	}

	/* Success! */
	return (0);

err0:
	/* Failure! */
	return (-1);
}

/* Wait duration can be interrupted by signals. */
static int
wait_ms(size_t msec)
{
	struct timespec ts;

	/* Try to wait for the desired duration. */
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	nanosleep(&ts, NULL);

	/* Success! */
	return (0);
}

static int
chain_one(void)
{
	pthread_t thread;
	int in[2];
	int out[2];
	const char * msg = "ping";
	size_t msglen = strlen(msg) + 1;
	char * buf;
	ssize_t r;
	int rc;

	/* Initialize output message buffer. */
	if ((buf = malloc(msglen)) == NULL) {
		warnp("malloc");
		goto err0;
	}

	/* Create socket pairs for the input and output. */
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, in)) {
		warnp("socketpair");
		goto err1;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, out)) {
		warnp("socketpair");
		goto err1;
	}

	/* Link the two pairs with pushbits. */
	if (pushbits(in[1], out[0], &thread)) {
		warnp("pushbits");
		goto err1;
	}

	/* Wait for thread to start. */
	wait_ms(100);

	/*
	 * Send the message; this will not block because the message is
	 * small enough that it's entirely buffered within the kernel.
	 */
	if (noeintr_write(in[0], msg, msglen) != (ssize_t)msglen) {
		warnp("noeintr_write");
		goto err3;
	}

	/* Message was sent successfully; close the input. */
	if (close(in[0])) {
		warnp("close");
		goto err2;
	}

	/* Read message; assume that we'll get it all at once. */
	if ((r = read(out[1], buf, msglen)) == -1) {
		warnp("read");
		goto err2;
	}
	if (strncmp(buf, msg, msglen) != 0) {
		warn0("failed to get the (full?) message");
		goto err2;
	}

	/* Wait for thread to finish, then clean up. */
	if ((rc = pthread_join(thread, NULL)) != 0) {
		warn0("pthread_join: %s", strerror(rc));
		goto err1;
	}
	free(buf);

	/* Success! */
	return (0);

err3:
	pthread_cancel(thread);
err2:
	pthread_join(thread, NULL);
err1:
	free(buf);
err0:
	/* Failure! */
	return (-1);
}

static int
chain_two(void)
{
	pthread_t thread[2];
	int in[2];
	int middle[2];
	int out[2];
	const char * msg = "ping";
	size_t msglen = strlen(msg) + 1;
	char * buf;
	ssize_t r;
	int rc;

	/* Initialize output message buffer. */
	if ((buf = malloc(msglen)) == NULL) {
		warnp("malloc");
		goto err0;
	}

	/* Create socket pairs for the input, middle, and output. */
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, in)) {
		warnp("socketpair");
		goto err1;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, middle)) {
		warnp("socketpair");
		goto err1;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, out)) {
		warnp("socketpair");
		goto err1;
	}

	/* Link the pairs with pushbits. */
	if (pushbits(middle[1], out[0], &thread[1])) {
		warnp("pushbits");
		goto err1;
	}
	if (pushbits(in[1], middle[0], &thread[0])) {
		warnp("pushbits");
		goto err2;
	}

	/* Wait for thread to start. */
	wait_ms(100);

	/*
	 * Send the message; this will not block because the message is
	 * small enough that it's entirely buffered within the kernel.
	 */
	if (noeintr_write(in[0], msg, msglen) != (ssize_t)msglen) {
		warnp("noeintr_write");
		goto err3;
	}

	/* Message was sent successfully; close the input. */
	if (close(in[0])) {
		warnp("close");
		goto err3;
	}

	/* Shut down the first thread. */
	if ((rc = pthread_join(thread[0], NULL)) != 0) {
		warn0("pthread_join: %s", strerror(rc));
		goto err2;
	}

	/* Read message; assume that we'll get it all at once. */
	if ((r = read(out[1], buf, msglen)) == -1) {
		warnp("read");
		goto err2;
	}
	if (strncmp(buf, msg, msglen) != 0) {
		warn0("failed to get the (full?) message");
		goto err2;
	}

	/* Wait for the second thread to finish, then clean up. */
	if ((rc = pthread_join(thread[1], NULL)) != 0) {
		warn0("pthread_join: %s", strerror(rc));
		goto err1;
	}
	free(buf);

	/* Success! */
	return (0);

err3:
	pthread_cancel(thread[0]);
	pthread_join(thread[0], NULL);
err2:
	pthread_cancel(thread[1]);
	pthread_join(thread[1], NULL);
err1:
	free(buf);
err0:
	/* Failure! */
	return (-1);
}

static int
echo_start_stop(void)
{
	pthread_t thread;
	int rc;

	/* Start echoing. */
	if (pushbits(STDIN_FILENO, STDOUT_FILENO, &thread)) {
		warnp("pushbits");
		goto err0;
	}

	/* Cancel the thread immediately. */
	if ((rc = pthread_cancel(thread)) != 0) {
		warn0("pthread_cancel: %s", strerror(rc));
		goto err0;
	}

	/* Wait for thread to finish. */
	if ((rc = pthread_join(thread, NULL)) != 0) {
		warn0("pthread_join: %s", strerror(rc));
		goto err0;
	}

	/* Success! */
	return (0);

err0:
	/* Failure! */
	return (-1);
}

static void
usage(void)
{

	fprintf(stderr, "usage: test_pushbits [-i filename -o filename] NUM\n");
	exit(1);
}

int
main(int argc, char ** argv)
{
	const char * ch;
	const char * filename_in = NULL;
	const char * filename_out = NULL;
	int desired_test;

	WARNP_INIT;

	/* Parse the command line. */
	while ((ch = GETOPT(argc, argv)) != NULL) {
		GETOPT_SWITCH(ch) {
		GETOPT_OPTARG("-i"):
			filename_in = optarg;
			break;
		GETOPT_OPTARG("-o"):
			filename_out = optarg;
			break;
		GETOPT_MISSING_ARG:
			warn0("Missing argument to %s", ch);
			usage();
		GETOPT_DEFAULT:
			warn0("illegal option -- %s", ch);
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* We should have processed all the arguments except for one. */
	if (argc != 1)
		usage();
	if (PARSENUM(&desired_test, argv[0], 1, 5)) {
		warnp("parsenum");
		goto err0;
	}

	/* Run the desired test. */
	switch(desired_test) {
	case 1:
		if ((filename_in == NULL) || (filename_out == NULL))
			usage();
		if (basic_copy(filename_in, filename_out))
			goto err0;
		break;
	case 2:
		if (echo_stdin_stdout())
			goto err0;
		break;
	case 3:
		if (chain_one())
			goto err0;
		break;
	case 4:
		if (chain_two())
			goto err0;
		break;
	case 5:
		if (echo_start_stop())
			goto err0;
		break;
	default:
		warn0("invalid test number");
		goto err0;
	}

	/* Success! */
	exit(0);

err0:
	/* Failure! */
	exit(1);
}
