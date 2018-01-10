#pragma once

#include "socks.hpp"

class jpsock;

class base_socket
{
public:
	virtual bool set_hostname(const char* sAddr) = 0;
	virtual bool connect() = 0;
	virtual int recv(char* buf, unsigned int len) = 0;
	virtual bool send(const char* buf) = 0;
	virtual void close(bool free) = 0;
};

class plain_socket : public base_socket
{
public:
	plain_socket(jpsock* err_callback);

	bool set_hostname(const char* sAddr);
	bool connect();
	int recv(char* buf, unsigned int len);
	bool send(const char* buf);
	void close(bool free);

private:
	jpsock* pCallback;
	addrinfo *pSockAddr;
	addrinfo *pAddrRoot;
	SOCKET hSocket;
};

