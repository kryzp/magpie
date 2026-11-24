#ifndef NET_NET_H
#define NET_NET_H

#include "core/types.h"

#include <memory>
#include "container/string.h"
#include "container/vector.h"

#define NET_MAX_MESSAGE_SIZE Megabytes(4)

namespace net {

class Library {
public:
	Library();
	~Library();

	Library(const Library &) = delete;
	Library &operator=(const Library &) = delete;

	bool IsReady() const
	{
		return ready;
	}

private:
	bool ready;
};

enum Delivery {
	Delivery_Reliable, // . . . ordered and guaranteed
	Delivery_Unreliable, // . . may drop or reorder (on TCP this is just Delivery_Reliable)
};

class IConnection {
public:
	virtual ~IConnection() = default;

	virtual void Send(const void *data, u32 size, Delivery delivery) = 0;
	virtual bool Receive(Vector<u8> *out_message) = 0; // false if no message

	virtual void Update() = 0;

	[[nodiscard]] virtual bool IsOpen() const = 0;
	virtual void Close() = 0;

	[[nodiscard]] virtual u32 GetID() const = 0;
	[[nodiscard]] virtual String GetRemoteAddress() const = 0;

	void Send(const Vector<u8> &data, Delivery delivery = Delivery_Reliable)
	{
		Send(data.data(), data.size(), delivery);
	}
};

class IListener {
public:
	virtual ~IListener() = default;
	virtual Unique<IConnection> Accept() = 0;
	[[nodiscard]] virtual bool IsOpen() const = 0;
	virtual void Close() = 0;
};

Unique<IConnection> ConnectTcp(const String &host, u16 port, u32 timeout_ms, String *error);
Unique<IListener> ListenTcp(u16 port, String *error);

}

#endif // NET_NET_H
