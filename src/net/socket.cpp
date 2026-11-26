#include "net/socket.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#define NET_INVALID_SOCKET           ((net::SocketHandle)INVALID_SOCKET)
#define NET_CLOSE_SOCKET(s)          closesocket((SOCKET)(s))
#define NET_LAST_ERROR()             WSAGetLastError()
#define NET_WOULD_BLOCK(e)           ((e) == WSAEWOULDBLOCK)
#define NET_IN_PROGRESS(e)           ((e) == WSAEWOULDBLOCK || (e) == WSAEINPROGRESS)

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#define NET_INVALID_SOCKET           (-1)
#define NET_CLOSE_SOCKET(s)          ::close(s)
#define NET_LAST_ERROR()             errno
#define NET_WOULD_BLOCK(e)           ((e) == EWOULDBLOCK || (e) == EAGAIN)
#define NET_IN_PROGRESS(e)           ((e) == EINPROGRESS)

#endif

#if defined(_WIN32)
typedef SOCKET NativeSocket;
#else
typedef int NativeSocket;
#endif

String net::LastSocketError()
{
	char buf[64];
	snprintf(buf, sizeof(buf), "socket error %d", NET_LAST_ERROR());
	return String(buf);
}

net::Socket::Socket()
	: handle(NET_INVALID_SOCKET)
{
}

net::Socket::Socket(SocketHandle handle_)
	: handle(handle_)
{
}

net::Socket::~Socket()
{
	Close();
}

net::Socket::Socket(Socket &&other) noexcept
	: handle(other.handle)
{
	other.handle = NET_INVALID_SOCKET;
}

net::Socket &net::Socket::operator=(Socket &&other) noexcept
{
	if (this != &other) {
		Close();
		handle = other.handle;
		other.handle = NET_INVALID_SOCKET;
	}

	return *this;
}

bool net::Socket::IsValid() const
{
	return handle != NET_INVALID_SOCKET;
}

void net::Socket::Close()
{
	if (handle != NET_INVALID_SOCKET) {
		NET_CLOSE_SOCKET(handle);
		handle = NET_INVALID_SOCKET;
	}
}

bool net::Socket::SetNonBlocking()
{
#if defined(_WIN32)
	u_long mode = 1;
	return ioctlsocket((SOCKET)handle, FIONBIO, &mode) == 0;
#else
	int flags = fcntl(handle, F_GETFL, 0);
	return flags >= 0 && fcntl(handle, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

void net::Socket::SetNoDelay()
{
	int one = 1;
	setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

bool net::Socket::ConnectTcp(const String &host, u16 port, u32 timeout_ms, String *error)
{
	Close();

	addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char port_str[16];
	snprintf(port_str, sizeof(port_str), "%u", port);

	addrinfo *results = nullptr;

	if (getaddrinfo(host.c_str(), port_str, &hints, &results) != 0 || !results) {
		if (error)
			*error = "could not resolve host '" + host + "'";

		return false;
	}

	bool connected = false;

	for (addrinfo *ai = results; ai && !connected; ai = ai->ai_next) {
		SocketHandle s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

		if (s == NET_INVALID_SOCKET)
			continue;

		handle = s;

		if (!SetNonBlocking()) {
			Close();
			continue;
		}

		int rc = connect(handle, ai->ai_addr, ai->ai_addrlen);

		if (rc == 0) {
			connected = true;
			break;
		}

		int err = NET_LAST_ERROR();

		if (!NET_IN_PROGRESS(err)) {
			Close();
			continue;
		}

#if defined(_WIN32)
		fd_set write_set;
		fd_set except_set;
		FD_ZERO(&write_set);
		FD_ZERO(&except_set);
		FD_SET((SOCKET)handle, &write_set);
		FD_SET((SOCKET)handle, &except_set);

		timeval tv;
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		rc = select(0, nullptr, &write_set, &except_set, &tv);

		if (rc > 0 && FD_ISSET((SOCKET)handle, &write_set))
			connected = true;
#else
		pollfd pfd;
		pfd.fd = handle;
		pfd.events = POLLOUT;
		pfd.revents = 0;

		rc = poll(&pfd, 1, static_cast<int>(timeout_ms));

		if (rc > 0 && (pfd.revents & POLLOUT)) {
			int so_error = 0;
			socklen_t len = sizeof(so_error);
			getsockopt(handle, SOL_SOCKET, SO_ERROR, &so_error, &len);
			connected = so_error == 0;
		}
#endif

		if (!connected)
			Close();
	}

	freeaddrinfo(results);

	if (!connected) {
		if (error)
			*error = "could not connect to " + host + ":" + port_str;

		return false;
	}

	SetNoDelay();

	return true;
}

bool net::Socket::ListenTcp(u16 port, String *error)
{
	Close();

	SocketHandle s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (s == NET_INVALID_SOCKET) {
		if (error)
			*error = "socket() failed: " + LastSocketError();
		return false;
	}

	handle = s;

	int one = 1;
	setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(handle, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
		if (error)
			*error = "bind() failed: " + LastSocketError();

		Close();
		return false;
	}

	if (listen(handle, 16) != 0) {
		if (error)
			*error = "listen() failed: " + LastSocketError();

		Close();
		return false;
	}

	if (!SetNonBlocking()) {
		if (error)
			*error = "could not make listener non-blocking";

		Close();
		return false;
	}

	return true;
}

net::Socket net::Socket::Accept()
{
	if (!IsValid())
		return Socket();

	SocketHandle s = accept(handle, nullptr, nullptr);

	if (s == NET_INVALID_SOCKET)
		return Socket();

	Socket accepted(s);
	accepted.SetNonBlocking();
	accepted.SetNoDelay();

	return accepted;
}

net::SocketResult net::Socket::Send(const void *data, u64 size, u64 *out_sent)
{
	*out_sent = 0;

	if (!IsValid())
		return SocketResult_Closed;

#if defined(_WIN32)
	int flags = 0;
#else
	int flags = MSG_NOSIGNAL;
#endif

	long rc = send(handle, data, size, flags);

	if (rc >= 0) {
		*out_sent = static_cast<u64>(rc);
		return SocketResult_Ok;
	}

	int err = NET_LAST_ERROR();

	if (NET_WOULD_BLOCK(err))
		return SocketResult_WouldBlock;

	return SocketResult_Error;
}

net::SocketResult net::Socket::Receive(void *data, u64 size, u64 *out_received)
{
	*out_received = 0;

	if (!IsValid())
		return SocketResult_Closed;

	long rc = recv(handle, data, size, 0);

	if (rc > 0) {
		*out_received = static_cast<u64>(rc);
		return SocketResult_Ok;
	}

	if (rc == 0)
		return SocketResult_Closed;

	int err = NET_LAST_ERROR();

	if (NET_WOULD_BLOCK(err))
		return SocketResult_WouldBlock;

	return SocketResult_Error;
}

String net::Socket::GetRemoteAddress() const
{
	if (!IsValid())
		return "";

	sockaddr_storage addr = {};
	socklen_t len = sizeof(addr);

	if (getpeername(handle, reinterpret_cast<sockaddr *>(&addr), &len) != 0)
		return "";

	char host[128] = {};
	char serv[32] = {};

	if (getnameinfo(reinterpret_cast<sockaddr *>(&addr), len, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
		return "";

	return String(host) + ":" + serv;
}
