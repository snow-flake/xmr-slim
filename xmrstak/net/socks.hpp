#pragma once

/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <errno.h>
#include <string.h>
#if defined(__FreeBSD__)
#include <netinet/in.h> /* Needed for IPPROTO_TCP */
#endif

inline void sock_init() {}
typedef int SOCKET;

#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)

inline void sock_close(SOCKET s)
{
	shutdown(s, SHUT_RDWR);
	close(s);
}

inline const char* sock_strerror(char* buf, size_t len)
{
	buf[0] = '\0';

#if defined(__APPLE__) || defined(__FreeBSD__) || !defined(_GNU_SOURCE) || !defined(__GLIBC__)

	strerror_r(errno, buf, len);
	return buf;
#else
	return strerror_r(errno, buf, len);
#endif
}

inline const char* sock_gai_strerror(int err, char* buf, size_t len)
{
	buf[0] = '\0';
	return gai_strerror(err);
}
