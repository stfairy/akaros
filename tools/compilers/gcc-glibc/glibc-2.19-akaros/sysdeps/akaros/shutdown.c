/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */
#include <sys/types.h>
#include <unistd.h>

#include "plan9_sockets.h"

/* Supposed to shut down all or part of the connection open on socket FD.
   HOW determines what to shut down:
     0 = No more receptions;
     1 = No more transmissions;
     2 = No more receptions or transmissions.
   Returns 0 on success, -1 for errors.  */
int shutdown(int fd, int how)
{
	/* plan 9 doesn't do a shutdown.  if you shut it down, it's now closed. */
	return close(fd);
}
