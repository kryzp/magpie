#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "core/types.h"

#include "container/string.h"

namespace net {

#if defined(_WIN32)
typedef uptr SocketHandle;
#else
typedef int SocketHandle;
#endif

enum SocketResult {
	SocketResult_Ok,
	SocketResult_WouldBlock,
	SocketResult_Closed,
	SocketResult_Error
};

class Socket {
public:
	Socket();
	explicit Socket(SocketHandle handle);
	~Socket();

	Socket(Socket &&other) noexcept;
	Socket &operator=(Socket &&other) noexcept;

	Socket(const Socket &) = delete;
	Socket &operator=(const Socket &) = delete;

	[[nodiscard]] bool IsValid() const;
	void Close();

	bool ConnectTcp(const String &host, u16 port, u32 timeout_ms, String *error);
	bool ListenTcp(u16 port, String *error);
	Socket Accept();

	SocketResult Send(const void *data, u64 size, u64 *out_sent);
	SocketResult Receive(void *data, u64 size, u64 *out_received);

	[[nodiscard]] String GetRemoteAddress() const;

private:
	bool SetNonBlocking();
	void SetNoDelay();

	SocketHandle handle;
};

String LastSocketError();

}

#endif // NET_SOCKET_H
