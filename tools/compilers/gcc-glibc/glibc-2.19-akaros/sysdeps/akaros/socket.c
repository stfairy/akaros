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
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "plan9_sockets.h"

/* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  */
int __socket(int domain, int stype, int protocol)
{
	Rock *r;
	int cfd, fd, n;
	int pfd[2];
	char *net;
	char msg[128];

	switch (domain) {
		case PF_INET:
			/* get a free network directory */
			switch (stype) {
				case SOCK_DGRAM:
					net = "udp";
					cfd = open("/net/udp/clone", O_RDWR);
					/* All BSD UDP sockets are in 'headers' mode, where each packet has
					 * the remote addr:port, local addr:port and other info. */
					if (!(cfd < 0)) {
						n = snprintf(msg, sizeof(msg), "headers");
						n = write(cfd, msg, n);
						if (n < 0) {
							perror("UDP socket headers failed");
							return -1;
						}
					}
					break;
				case SOCK_STREAM:
					net = "tcp";
					cfd = open("/net/tcp/clone", O_RDWR);
					break;
				default:
					errno = EPROTONOSUPPORT;
					return -1;
			}
			if (cfd < 0) {
				return -1;
			}
			return _sock_data(cfd, net, domain, stype, protocol, 0);
		case PF_UNIX:
			if (pipe(pfd) < 0) {
				return -1;
			}
			r = _sock_newrock(pfd[0]);
			r->domain = domain;
			r->stype = stype;
			r->protocol = protocol;
			r->other = pfd[1];
			return pfd[0];
		default:
			errno = EPROTONOSUPPORT;
			return -1;
	}
}

weak_alias(__socket, socket)
