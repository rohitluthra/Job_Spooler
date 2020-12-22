		/**
		 * === DO NOT MODIFY THIS FILE ===
		 * If you need some other prototypes or constants in a header, please put them
		 * in another header file.
		 *
		 * When we grade, we will be replacing this file with our own copy.
		 * You have been warned.
		 * === DO NOT MODIFY THIS FILE ===
		 */

		/*
		 * This is a very basic implementation of a "readline" function.
		 * We would use GNU readline, except for the fact that it is impossible to
		 * use GNU readline together with properly written signal handling code.
		 * The version defined here arranges for race-free callbacks to a
		 * client-specified function before blocking for user input.
		 * This permits the actual signal handlers to be written in a safe manner
		 * in which they just set flags to indicate that signals have arrived,
		 * and the actual work of dealing with the signals to postponed to the
		 * callback function where there are no constraints on what functions can
		 * be safely used.
		 *
		 * Author: E. Stark, 10/2019.
		 */
		#include <stdlib.h>
		#include <stdio.h>
		#include <signal.h>
		#include <errno.h>
		#include <unistd.h>
		#include "../include/sf_readline.h"
		#include "../include/debug.h"

static signal_hook_func_t *signal_hook_func;

void sf_set_readline_signal_hook(signal_hook_func_t func)
{
	signal_hook_func = func;
}

		#define BUFSIZE 32  // Initial buffer size.

char *sf_readline(char *prompt)
{

	int size = BUFSIZE;
	char *buf = malloc(size);
	if(buf == NULL)
	{
		debug("malloc failed");
		return NULL;
	}
	char *bp = buf;
	fprintf(stdout, "%s", prompt);
	fflush(stdout);
	bp = buf;


	while(1)
	{
		char c;
			// Here we mask all signals, call a client-specified handler function (if any)
			// to deal with any signals that have already been noticed, then atomically
			// unmask signals and block until stdin is ready for reading.
		sigset_t mask, omask;


		/*
		- sigfillset() is part of a family of functions that manipulate signal sets.
		Signal sets are data objects that let a process keep track of groups of signals.
		- This function initializes the signal set set to all defined signals.

			It always returns .... zero
		*/
		sigfillset(&mask);

		/*
		- &mask is the sigset_t whose signals are defined.
		- SIG_BLOCK is the set of blocked signals in the union of the current set and the set argument

		- predefined syntax: int sigprocmask(int how, const sigset_ *set, sigset_t * oldset

		- sigprocmask() is used to fetch and/or change the signal mask of the
	       calling thread.  The signal mask is the set of signals whose delivery
	       is currently blocked for the caller (see also signal(7) for more
	       details).

	       The behavior of the call is dependent on the value of how, as
	       follows.

	       SIG_BLOCK
	              The set of blocked signals is the union of the current set and
	              the set argument.

	       SIG_UNBLOCK
	              The signals in set are removed from the current set of blocked
	              signals.  It is permissible to attempt to unblock a signal
	              which is not blocked.

	       SIG_SETMASK
	              The set of blocked signals is set to the argument set.
		)
		*/
		sigprocmask(SIG_BLOCK, &mask, &omask);
		if(signal_hook_func)
			signal_hook_func();


		/*
		- Use pselect() to atomically release signals and wait for input to become available.
		- select and pselect allow a program to monitor multiple file drsciptor, waiting until one or more
		- is ready for some class I?O operations (eg: input possible). A file descriptor is conidered ready if it is possible tto perform corresponding I/O
		 */

		/*
		fd_set: data type that represents file descriptor set for the "select" function. It is a bit array.
		*/
		fd_set readfds;

		// clears the set.
		FD_ZERO(&readfds);

		//Add or remove a given file descriptor froma set.
		FD_SET(0, &readfds);
		/*
		- int pselect (nfds, &reddfs, &writefds, &exceptdfs, timeout, &sigmask)
		- The reason that pselect() is needed is that if one wants to wait for either a signal or for a file descriptor to become ready,
		  then an atomic test is needed to prevent race conditions. (Suppose the signal handler sets a global flag and returns.
		  Then a test of this global flag followed by a call of select() could hang indefinitely if the signal arrived just after the test
		  but just before the call. By contrast, pselect() allows one to first block signals,
		  handle the signals that have come in, then call pselect() with the desired sigmask, avoiding the race.)

		*/

		int n = pselect(1, &readfds, NULL, NULL, NULL, &omask);
		int e = errno;

			// OK to unmask signals now that we know there is input to read and we
			// won't block trying to get it.
		sigprocmask(SIG_SETMASK, &omask, NULL);

		if(n < 0)
		{
			if(e == EINTR)
				continue;  // pselect() was interrupted by a signal, restart
			debug("select failed (errno = %d)", e);
			return NULL;
		}
		else if(n == 0)
		{

			debug("else if: %s\n", bp);

			if(bp - buf > 0)
			{
				debug("EOF on input -- treating as newline");
				break;
			} else {
				debug("EOF on input");
				free(buf);
				return NULL;
			}
		}
		else
		{
			// debug("I think i am coming here");
			    // Read one character.  That's all we can do without a risk of blocking.
			if(read(0, &c, 1) != 1)
			{
				debug("else %s\n", bp);

				if(bp - buf > 0)
				{
					debug("read returned <= 0 -- treating as newline");
					break;
				} else
				{
					debug("read returned <= 0 -- treating as EOF");
					free(buf);
					return NULL;
				}
			}
			if(c == '\n'){
				// debug("may beee");
				break;
			}
			if(bp - buf >= size -1)
			{
				size <<= 1;
				debug("realloc: %d", size);
				char *nbuf = realloc(buf, size);
				if(nbuf == NULL)
				{
					debug("realloc failed");
					break;
				}
				else
				{
					bp = nbuf + (bp - buf);
					buf = nbuf;
				}
			}

			*bp++ = c;
		}
	}

	//debug("%s\n", bp);

	*bp = '\0';
	return buf;
}
