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
#include <arpa/inet.h>

#include "plan9_sockets.h"

/* Open a connection on socket FD to peer at ADDR (which LEN bytes long).
   For connectionless socket types, just set the default address to send to
   and the only address from which to accept transmissions.
   Return 0 on success, -1 for errors.  */
int __connect(int fd, __CONST_SOCKADDR_ARG addr, socklen_t alen)
{
	Rock *r;
	int n, cfd, nfd;
	char msg[8 + 256 + 1], file[8 + 256 + 1];
	struct sockaddr_in *lip, *rip;
	struct sockaddr_un *runix;
	static int vers;

	r = _sock_findrock(fd, 0);
	if (r == 0) {
		errno = ENOTSOCK;
		return -1;
	}
	if (alen > sizeof(r->raddr)) {
		errno = ENAMETOOLONG;
		return -1;
	}
	memmove(&r->raddr, addr.__sockaddr__, alen);

	switch (r->domain) {
		case PF_INET:
			/* UDP sockets are already announced (during bind), so we can't issue
			 * a connect message.  Either connect or announce, not both.  All sends
			 * will later do a sendto, based off the contents of r->raddr, so we're
			 * already done here */
			if (r->stype == SOCK_DGRAM)
				return 0;
			/* set up a tcp or udp connection */
			cfd = open(r->ctl, O_RDWR);
			if (cfd < 0) {
				return -1;
			}
			/* whatever .. */
			rip = (struct sockaddr_in *)addr.__sockaddr_in__;
			lip = (struct sockaddr_in *)&r->addr;
			if (lip->sin_port)
				snprintf(msg, sizeof msg, "connect %s!%d%s %d",
						 inet_ntoa(rip->sin_addr), ntohs(rip->sin_port),
						 r->reserved ? "!r" : "", ntohs(lip->sin_port));
			else
				snprintf(msg, sizeof msg, "connect %s!%d%s",
						 inet_ntoa(rip->sin_addr), ntohs(rip->sin_port),
						 r->reserved ? "!r" : "");
			n = write(cfd, msg, strlen(msg));
			if (n < 0) {
				close(cfd);
				return -1;
			}
			close(cfd);
			return 0;
		case PF_UNIX:
			/* null terminate the address */
			if (alen == sizeof(r->raddr))
				alen--;
			*(((char *)&r->raddr) + alen) = 0;

			if (r->other < 0) {
				errno = EINVAL;	//EGREG;
				return -1;
			}

			/* put far end of our pipe in /srv */
			snprintf(msg, sizeof msg, "UD.%d.%d", getpid(), vers++);
			if (_sock_srv(msg, r->other) < 0) {
				r->other = -1;
				return -1;
			}
			r->other = -1;

			/* tell server the /srv file to open */
			runix = (struct sockaddr_un *)&r->raddr;
			_sock_srvname(file, runix->sun_path);
			nfd = open(file, O_RDWR);
			if (nfd < 0) {
				unlink(msg);
				return -1;
			}
			if (write(nfd, msg, strlen(msg)) < 0) {
				close(nfd);
				unlink(msg);
				return -1;
			}
			close(nfd);

			/* wait for server to open it and then remove it */
			read(fd, file, sizeof(file));
			_sock_srvname(file, msg);
			unlink(file);
			return 0;
		default:
			errno = EAFNOSUPPORT;
			return -1;
	}
}
weak_alias(__connect, connect)
libc_hidden_def(__connect)
