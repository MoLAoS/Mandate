// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//				  2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_SOCKET_H_
#define _SHARED_PLATFORM_SOCKET_H_

#include <string>
#include <cassert>

#include "projectConfig.h"

#ifdef HAVE_BYTESWAP_H
#	include <byteswap.h>
#endif

#ifdef USE_POSIX_SOCKETS
#	include <unistd.h>
#	include <errno.h>
#	include <sys/socket.h>
#	include <sys/types.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <fcntl.h>

#	include "types.h"
#	include "util.h"

#elif defined(WIN32) || defined(WIN64)
#	include <winsock.h>

#	include "types.h"
#	include "util.h"

#	define uint16_t uint16
#	define uint32_t uint32
#	define uint64_t uint64
#endif

using std::string;

namespace Test {
	class CircularBufferTest;
}

namespace Shared { namespace Platform {

// =====================================================
//	class IP
// =====================================================

class Ip {
private:
	unsigned char bytes[4];

public:
	Ip();
	Ip(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3);
	Ip(const string& ipString);

	unsigned char getByte(int byteIndex)	{return bytes[byteIndex];}
	string getString() const;
};

class SocketException : public runtime_error {
public:
	SocketException(const string &msg) : runtime_error(msg){}
};

// =====================================================
//	class Socket
// =====================================================

class Socket {
	friend class Test::CircularBufferTest;
#if defined(WIN32) || defined(WIN64)
	class LibraryManager {
	public:
		LibraryManager();
		~LibraryManager();
	};

	static LibraryManager libraryManager;
#endif

private:
	typedef char* char_ptr;

	class CircularBuffer {
		int buffer_size;

		char_ptr buffer;
		size_t tail, head;
		bool full;

	public:
		CircularBuffer(int size = 8 * 1024) 
				: buffer_size(size), buffer(0)
				, tail(0), head(0), full(false) {
			assert(buffer_size > 0);
			buffer = new char[buffer_size]; 
		}
		~CircularBuffer() { delete [] buffer; }

		/** retrieve the current write position */
		char_ptr getWritePos() const { return buffer + head; }

		void operator+=(size_t b);	/**< record that we have added b bytes to the buffer */
		void operator-=(size_t b);	/**< record that we have removed b bytes from the buffer */

		/** Number of bytes available to read */
		size_t bytesAvailable() const;

		/** peek the next n bytes, copying them to dst if the request can be satisfied
		  * @return true if ok, false if not enough bytes are available */
		bool peekBytes(void *dst, size_t n);	

		/** read n bytes to dst, advancing tail offset ('removing' them from the buffer)
		  * @return true if all ok, false if not enough bytes available, in which case none will be read */
		bool readBytes(void *dst, int n);

		/** returns the maximum write length from the head, if no more bytes can be written
		  * after this (because the head would be at the tail) limit is set to true */
		int getMaxWrite(bool &limit) const;

		/** free space in buffer */
		size_t getFreeBytes() const;
	};

protected:
	SOCKET sock;
	SOCKET udpsockfd;
	CircularBuffer buffer;

public:
	Socket(SOCKET sock);
	Socket();
	~Socket();

	int getDataToRead();
	int send(const void *data, int dataSize);
	bool receive(void *data, int dataSize);
	bool peek(void *data, int dataSize);
	bool skip(int skipSize);

	void setNoDelay();
	void setBlock(bool block);
	bool isReadable();
	bool isWritable();
	bool isConnected();

	string getHostName() const;
	string getIp() const;

	void close();

protected:
	void handleError(const char *caller) const;
	void readAll();
};

// =====================================================
//	class ClientSocket
// =====================================================

class ClientSocket: public Socket {
private:
	bool udpBound;

public:
	ClientSocket() : udpBound(false) {}

	void connect(const Ip &ip, int port);

	/** @return ip of the sender 
	  * @throws SocketException when socket is no longer receiving */
	Ip receiveAnnounce(int port, char *hostname, int dataSize);
	void disconnectUdp() { closesocket(udpsockfd); }
};

// =====================================================
//	class ServerSocket
// =====================================================

class ServerSocket: public Socket {
private:
	Ip broadcastAddr;

public:
	ServerSocket() {
		Ip ip(getIp());
		//TODO: allow a netmask to decide addr
		broadcastAddr = Ip(ip.getByte(0), ip.getByte(1), ip.getByte(2), 255);
	}

	/** sends a udp packet with hostname as the message */
	//TODO: use a network message instead so when sending to master server it can include other details
	void sendAnnounce(int port);

	void bind(int port);
	void listen(int connectionQueueSize= SOMAXCONN);
	Socket *accept();


};

}}//end namespace

#endif
