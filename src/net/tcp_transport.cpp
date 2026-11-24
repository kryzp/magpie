#include "net/tcp_transport.h"
#include "core/log.h"

#include <string.h>
#include <utility>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#endif

net::Library::Library()
	: ready(true)
{
#if defined(_WIN32)
	WSADATA data;
	ready = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#endif
}

net::Library::~Library()
{
#if defined(_WIN32)
	if (ready)
		WSACleanup();
#endif
}

net::TcpConnection::TcpConnection(Socket &&socket_, u32 id_)
	: socket(std::move(socket_))
	, id(id_)
	, open(true)
	, send_offset(0)
{
	remote_address = socket.GetRemoteAddress();
}

net::TcpConnection::~TcpConnection()
{
	Close();
}

void net::TcpConnection::Send(const void *data, u32 size, Delivery delivery)
{
	if (!open)
		return;

	u8 header[4] = {
		(u8)(size >> 24),
		(u8)(size >> 16),
		(u8)(size >> 8),
		(u8)(size)
	};

	send_buffer.insert(send_buffer.end(), header, header + 4);
	send_buffer.insert(send_buffer.end(), (const u8 *)data, (const u8 *)data + size);
}

bool net::TcpConnection::Receive(Vector<u8> *out_message)
{
	if (inbox.empty())
		return false;

	*out_message = std::move(inbox.front());
	inbox.pop_front();

	return true;
}

void net::TcpConnection::Flush()
{
	while (open && send_offset < send_buffer.size()) {
		u64 sent = 0;
		SocketResult result = socket.Send(send_buffer.data() + send_offset, send_buffer.size() - send_offset, &sent);

		if (result == SocketResult_Ok) {
			send_offset += sent;
			continue;
		}

		if (result == SocketResult_WouldBlock)
			break;

		Close();
		return;
	}

	if (send_offset > 0 && send_offset == send_buffer.size()) {
		send_buffer.clear();
		send_offset = 0;
	} else if (send_offset > Kilobytes(64)) {
		send_buffer.erase(send_buffer.begin(), send_buffer.begin() + (u64)send_offset);
		send_offset = 0;
	}
}

void net::TcpConnection::Fill()
{
	u8 chunk[16384];

	while (open) {
		u64 received = 0;
		SocketResult result = socket.Receive(chunk, sizeof(chunk), &received);

		if (result == SocketResult_Ok) {
			recv_buffer.insert(recv_buffer.end(), chunk, chunk + received);
			continue;
		}

		if (result == SocketResult_WouldBlock)
			break;

		Close();
		return;
	}
}

void net::TcpConnection::ExtractMessages()
{
	u64 at = 0;

	while (recv_buffer.size() - at >= 4) {
		const u8 *h = recv_buffer.data() + at;
		u32 size = ((u32)h[0] << 24) | ((u32)h[1] << 16) | ((u32)h[2] << 8) | (u32)h[3];

		if (size > NET_MAX_MESSAGE_SIZE) {
			Close();
			return;
		}

		if (recv_buffer.size() - at - 4 < size)
			break;

		inbox.emplace_back(recv_buffer.begin() + (u64)(at + 4), recv_buffer.begin() + (u64)(at + 4 + size));
		at += 4 + size;
	}

	if (at > 0)
		recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + (u64)at);
}

void net::TcpConnection::Update()
{
	if (!open)
		return;

	Flush();
	Fill();
	ExtractMessages();
}

void net::TcpConnection::Close()
{
	if (!open)
		return;

	// push out whatever is still queued
	while (send_offset < send_buffer.size()) {
		u64 sent = 0;

		if (socket.Send(send_buffer.data() + send_offset, send_buffer.size() - send_offset, &sent) != SocketResult_Ok || sent == 0)
			break;

		send_offset += sent;
	}

	open = false;
	socket.Close();
}

net::TcpListener::TcpListener(Socket &&socket_)
	: socket(std::move(socket_))
	, next_id(1)
{
}

net::TcpListener::~TcpListener()
{
	Close();
}

Unique<net::IConnection> net::TcpListener::Accept()
{
	Socket accepted = socket.Accept();

	if (!accepted.IsValid())
		return nullptr;

	return CreateScope<TcpConnection>(std::move(accepted), next_id++);
}

Unique<net::IConnection> net::ConnectTcp(const String &host, u16 port, u32 timeout_ms, String *error)
{
	Socket socket;

	if (!socket.ConnectTcp(host, port, timeout_ms, error))
		return nullptr;

	return CreateScope<TcpConnection>(std::move(socket), 0);
}

Unique<net::IListener> net::ListenTcp(u16 port, String *error)
{
	Socket socket;

	if (!socket.ListenTcp(port, error))
		return nullptr;

	return CreateScope<TcpListener>(std::move(socket));
}
