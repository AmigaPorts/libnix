#include "socket.h"

#include <exec/libraries.h>

/* Roadshow's address functions, through the global bsdsocket.library base. */
extern struct Library * SocketBase;

void freeaddrinfo(struct addrinfo *ai) {
	__freeaddrinfo_base(SocketBase, ai);
}

int getaddrinfo(const char *restrict nodename,
       const char *restrict servname,
       const struct addrinfo *restrict hints,
       struct addrinfo **restrict res) {
	return __getaddrinfo_base(SocketBase, nodename, servname, hints, res);
}

int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen, int flags) {
	return __getnameinfo_base(SocketBase, addr, addrlen, host, hostlen, serv, servlen, flags);
}

const char * gai_strerror(int ecode) {
	return (const char *)__gai_strerror_base(SocketBase, ecode);
}
