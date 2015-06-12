/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */
/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#include "plan9_sockets.h"

/* Put the local address of FD into *ADDR and its length in *LEN.  */
int __getsockname(int fd, __SOCKADDR_ARG addr, socklen_t * __restrict alen)
{
	Rock *r;
	int i;
	struct sockaddr_in *lip;
	struct sockaddr_un *lunix;

	r = _sock_findrock(fd, 0);
	if (r == 0) {
		errno = ENOTSOCK;
		return -1;
	}

	switch (r->domain) {
		case PF_INET:
			lip = addr.__sockaddr_in__;
			_sock_ingetaddr(r, lip, alen, "local");
			break;
		case PF_UNIX:
			lunix = (struct sockaddr_un *)&r->addr;
			i = &lunix->sun_path[strlen(lunix->sun_path)] - (char *)lunix;
			memmove(addr.__sockaddr_un__, lunix, i);
			*alen = i;
			break;
		default:
			errno = EAFNOSUPPORT;
			return -1;
	}
	return 0;
}

weak_alias(__getsockname, getsockname)
