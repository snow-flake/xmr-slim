//
// Created by Snow Flake on 1/21/18.
//

#include "plain_socket.h"
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <cstring>


inline void sock_close(SOCKET s) {
	shutdown(s, SHUT_RDWR);
	close(s);
}


inline const char *sock_strerror(char *buf, size_t len) {
	buf[0] = '\0';
#if defined(__APPLE__) || defined(__FreeBSD__) || !defined(_GNU_SOURCE) || !defined(__GLIBC__)
	strerror_r(errno, buf, len);
	return buf;
#else
	return strerror_r(errno, buf, len);
#endif
}


inline const char *sock_gai_strerror(int err, char *buf, size_t len) {
	buf[0] = '\0';
	return gai_strerror(err);
}


plain_socket::plain_socket(socket_wrapper *err_callback) : pCallback(err_callback) {
	hSocket = INVALID_SOCKET;
	pSockAddr = nullptr;
}

bool plain_socket::set_hostname(const char *sAddr) {
	char sAddrMb[256];
	char *sTmp, *sPort;

	size_t ln = strlen(sAddr);
	if (ln >= sizeof(sAddrMb)){
		pCallback->set_socket_error("CONNECT error: Pool address overflow.");
		return false;
	}

	memcpy(sAddrMb, sAddr, ln);
	sAddrMb[ln] = '\0';

	if ((sTmp = strstr(sAddrMb, "//")) != nullptr) {
		sTmp += 2;
		memmove(sAddrMb, sTmp, strlen(sTmp) + 1);
	}

	if ((sPort = strchr(sAddrMb, ':')) == nullptr) {
		pCallback->set_socket_error("CONNECT error: Pool port number not specified, please use format <hostname>:<port>.");
		return false;
	}

	sPort[0] = '\0';
	sPort++;

	addrinfo hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	pAddrRoot = nullptr;
	int err;
	if ((err = getaddrinfo(sAddrMb, sPort, &hints, &pAddrRoot)) != 0) {
		char sSockErrText[512];
		const char * msg = sock_gai_strerror(err, sSockErrText, sizeof(sSockErrText));
		pCallback->set_socket_error(std::string("CONNECT error: GetAddrInfo: ") + std::string(msg));
		return false;
	}

	addrinfo *ptr = pAddrRoot;
	std::vector<addrinfo *> ipv4;
	std::vector<addrinfo *> ipv6;

	while (ptr != nullptr) {
		if (ptr->ai_family == AF_INET) {
			ipv4.push_back(ptr);
		}
		if (ptr->ai_family == AF_INET6) {
			ipv6.push_back(ptr);
		}
		ptr = ptr->ai_next;
	}

	if (ipv4.empty() && ipv6.empty()) {
		freeaddrinfo(pAddrRoot);
		pAddrRoot = nullptr;
		pCallback->set_socket_error("CONNECT error: I found some DNS records but no IPv4 or IPv6 addresses.");
		return false;
	} else if (!ipv4.empty() && ipv6.empty()) {
		pSockAddr = ipv4[rand() % ipv4.size()];
	} else if (ipv4.empty() && !ipv6.empty()) {
		pSockAddr = ipv6[rand() % ipv6.size()];
	} else if (!ipv4.empty() && !ipv6.empty()) {
		pSockAddr = ipv4[rand() % ipv4.size()];
	}

	hSocket = socket(pSockAddr->ai_family, pSockAddr->ai_socktype, pSockAddr->ai_protocol);
	if (hSocket == INVALID_SOCKET) {
		freeaddrinfo(pAddrRoot);
		pAddrRoot = nullptr;
		pCallback->set_socket_error("CONNECT error: Socket creation failed ");
		return false;
	}

	return true;
}

bool plain_socket::connect() {
	int ret = ::connect(hSocket, pSockAddr->ai_addr, (int) pSockAddr->ai_addrlen);

	freeaddrinfo(pAddrRoot);
	pAddrRoot = nullptr;

	if (ret != 0) {
		pCallback->set_socket_error("CONNECT error: ");
		return false;
	}
	else
		return true;
}

int plain_socket::recv(char *buf, unsigned int len) {
	int ret = ::recv(hSocket, buf, len, 0);
	if (ret == 0) {
		pCallback->set_socket_error("RECEIVE error: socket closed");
	}
	if (ret == SOCKET_ERROR || ret < 0) {
		pCallback->set_socket_error("RECEIVE error: ");
	}

	return ret;
}

bool plain_socket::send(const char *buf) {
	int pos = 0, slen = strlen(buf);
	while (pos != slen) {
		int ret = ::send(hSocket, buf + pos, slen - pos, 0);
		if (ret == SOCKET_ERROR) {
			pCallback->set_socket_error("SEND error: ");
			return false;
		} else
			pos += ret;
	}

	return true;
}

void plain_socket::close(bool free) {
	if (hSocket != INVALID_SOCKET) {
		sock_close(hSocket);
		hSocket = INVALID_SOCKET;
	}
}
