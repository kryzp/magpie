#ifndef NET_TCP_TRANSPORT_H
#define NET_TCP_TRANSPORT_H

#include "core/types.h"
#include "net/net.h"
#include "net/socket.h"
#include "container/deque.h"
#include "container/vector.h"

namespace net {

class TcpConnection : public IConnection {
public:
	TcpConnection(Socket &&socket, u32 id);
	~TcpConnection() override;

	void Send(const void *data, u32 size, Delivery delivery) override;
	bool Receive(Vector<u8> *out_message) override;
	void Update() override;

	[[nodiscard]] bool IsOpen() const override
	{
		return open;
	}

	void Close() override;

	[[nodiscard]] u32 GetID() const override
	{
		return id;
	}

	[[nodiscard]] String GetRemoteAddress() const override
	{
		return remote_address;
	}

private:
	void Flush();
	void Fill();
	void ExtractMessages();

	Socket socket;
	u32 id;
	bool open;
	String remote_address;

	Vector<u8> send_buffer;
	u64 send_offset;

	Vector<u8> recv_buffer;
	Deque<Vector<u8>> inbox;
};

class TcpListener : public IListener {
public:
	explicit TcpListener(Socket &&socket);
	~TcpListener() override;

	Unique<IConnection> Accept() override;

	[[nodiscard]] bool IsOpen() const override
	{
		return socket.IsValid();
	}

	void Close() override
	{
		socket.Close();
	}

private:
	Socket socket;
	u32 next_id;
};

}

#endif // NET_TCP_TRANSPORT_H
