//
// Created by Snow Flake on 1/21/18.
//

#ifndef XMR_STAK_PLAIN_SOCKET_H
#define XMR_STAK_PLAIN_SOCKET_H

#include <string>
#include <algorithm>
#include <sys/socket.h> /* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */

#if defined(__FreeBSD__)
#include <netinet/in.h> /* Needed for IPPROTO_TCP */
#endif


typedef int SOCKET;
#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)


class base_socket {
public:
	virtual bool set_hostname(const char *sAddr) = 0;
	virtual bool connect() = 0;
	virtual int recv(char *buf, unsigned int len) = 0;
	virtual bool send(const char *buf) = 0;
	virtual void close(bool free) = 0;
};

class socket_wrapper {
public:
	virtual void set_socket_error(const std::string & error) = 0;
};

class plain_socket : public base_socket {
public:
	plain_socket(socket_wrapper *err_callback);

	bool set_hostname(const char *sAddr);

	bool connect();

	int recv(char *buf, unsigned int len);

	bool send(const char *buf);

	void close(bool free);

private:
	socket_wrapper *pCallback;
	addrinfo *pSockAddr;
	addrinfo *pAddrRoot;
	SOCKET hSocket;
};


#endif //XMR_STAK_PLAIN_SOCKET_H
