// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martiño Figueroa
//				  2005		Matthias Braun <matze@braunis.de>
//				  2010		James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "socket.h"

#include "conversion.h"
#include "leak_dumper.h"

using namespace Shared::Util;

#ifndef SOCKET_ERROR
#	define SOCKET_ERROR (-1)
#endif

#ifndef INVALID_SOCKET
#	define INVALID_SOCKET (-1)
#endif

#if defined(HAVE_SYS_IOCTL_H)
#	define BSD_COMP /* needed for FIONREAD on Solaris2 */
#	include <sys/ioctl.h>
#endif

#if defined(HAVE_SYS_FILIO_H) /* needed for FIONREAD on Solaris 2.5 */
#	include <sys/filio.h>
#endif

#ifdef USE_POSIX_SOCKETS
#	include <netinet/tcp.h>
#	define socket_close ::close
#	define get_error() errno
#	define WOULD_BLOCK EAGAIN
#	define TIMEVAL struct timeval
#elif defined(WIN32)
#	define socket_close closesocket
#	define get_error() WSAGetLastError()
#	define WOULD_BLOCK WSAEWOULDBLOCK
	typedef int ssize_t;
	typedef int socklen_t;
#else
#	error not Windows and USE_POSIX_SOCKETS not defined
#endif

namespace Shared{ namespace Platform{

// =====================================================
//	class Ip
// =====================================================

Ip::Ip(){
	bytes[0]= 0;
	bytes[1]= 0;
	bytes[2]= 0;
	bytes[3]= 0;
}

Ip::Ip(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3){
	bytes[0]= byte0;
	bytes[1]= byte1;
	bytes[2]= byte2;
	bytes[3]= byte3;
}


Ip::Ip(const string& ipString){
	int offset = 0;
	for(int byteIndex = 0; byteIndex < 4; ++byteIndex) {
		int dotPos = ipString.find_first_of('.', offset);
		bytes[byteIndex] = atoi(ipString.substr(offset, dotPos - offset).c_str());
		offset = dotPos + 1;
	}
}

string Ip::getString() const{
	return intToStr(bytes[0]) + "." + intToStr(bytes[1]) + "." + intToStr(bytes[2]) + "." + intToStr(bytes[3]);
}

#if defined(WIN32)
	// =====================================================
	//	class Socket::LibraryManager
	// =====================================================

	Socket::LibraryManager Socket::libraryManager;

	Socket::LibraryManager::LibraryManager(){
		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD(2, 0);
		WSAStartup(wVersionRequested, &wsaData);
		//dont throw exceptions here, this is a static initializacion
	}

	Socket::LibraryManager::~LibraryManager(){
		WSACleanup();
	}
#endif

// =====================================================
//	class Socket::CircularBuffer
// =====================================================

void Socket::CircularBuffer::operator+=(size_t b) {
	assert(b && head + b <= buffer_size);
	head += b;
	if (head == buffer_size) head = 0;
	if (head == tail) full = true;
}

void Socket::CircularBuffer::operator-=(size_t b) {
	assert(b);
	if (tail + b <= buffer_size) {
		tail += b;
		if (tail == buffer_size) tail = 0;
	} else {
		tail = b - (buffer_size - tail);
	}
	if (full) full = false;
}

size_t Socket::CircularBuffer::bytesAvailable() const {
	if (full) {
		return buffer_size;
	} else if (head < tail) {
		return head + buffer_size - tail;
	} else {
		return head - tail;
	}
}

bool Socket::CircularBuffer::peekBytes(void *dst, size_t n) {
	if (bytesAvailable() < n) {
		return false;
	}
	char_ptr ptr = (char_ptr)dst;
	if (tail + n <= buffer_size) {
		memcpy(ptr, buffer + tail, n);
	} else {
		size_t first_n = buffer_size - tail;
		size_t second_n = n - first_n;
		memcpy(ptr, buffer + tail, first_n);
		memcpy(ptr + first_n, buffer, second_n);
	}
	return true;
}

bool Socket::CircularBuffer::readBytes(void *dst, int n) {
	if (peekBytes(dst, n)) {
		*this -= n;
		return true;
	}
	return false;
}

int Socket::CircularBuffer::getMaxWrite(bool &limit) const {
	if (full) {
		limit = true;
		return 0;
	}
	if (tail > head) { // chasing tail?
		limit = true;
		return tail - head;
	} else {
		limit = tail ? false : true;
		return buffer_size - head;
	}
}

size_t Socket::CircularBuffer::getFreeBytes() const {
	if (full) return 0;
	if (tail > head) {
		return tail - head;
	} else {
		return buffer_size - head + tail;
	}
}

// =====================================================
//	class Socket
// =====================================================

Socket::Socket(SOCKET sock){
	this->sock = sock;
}

Socket::Socket() {
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		throw SocketException("Error creating socket");
	}

	if((udpsockfd = socket(AF_INET,SOCK_DGRAM,0)) == INVALID_SOCKET) {
		throw runtime_error("Error creating udp socket");
	}
}

Socket::~Socket(){
	close();
	socket_close(udpsockfd);
}

void Socket::close() {
	if (sock != INVALID_SOCKET) {
		shutdown(sock, 2);
		socket_close(sock);
	}
}

int Socket::getDataToRead(){
	readAll();
	return buffer.bytesAvailable();
}

int Socket::send(const void *data, int dataSize) {
	ssize_t res = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);
	if (res == SOCKET_ERROR) {
		if (get_error() != WOULD_BLOCK) {
			handleError(__FUNCTION__);
		} else {

			// TODO
			// wait a little, then try again

		}
	}
	//cout << "sent message, " << res << " bytes.";
	return res;
}

void Socket::readAll() {
	bool limit;
	int n = buffer.getMaxWrite(limit);
	int r = recv(sock, buffer.getWritePos(), n, 0);
	if (r == SOCKET_ERROR && get_error() != WOULD_BLOCK) {
		handleError(__FUNCTION__);
	}
	if (r > 0) {
		buffer += r;
		if (r == n && !limit) {
			n = buffer.getMaxWrite(limit);
			assert(n);
			int r2 = recv(sock, buffer.getWritePos(), n, 0);
			if (r2 > 0) {
				buffer += r2;
			} else if (r2 == SOCKET_ERROR && get_error() != WOULD_BLOCK) {
				handleError(__FUNCTION__);
			}
			//cout << "Socket::readALL() read " << r + r2 << " bytes.\n";
			return;
		}
		//cout << "Socket::readALL() read " << r << " bytes.\n";
	}
}

bool Socket::receive(void *data, int dataSize) {
	readAll();
	if (buffer.readBytes(data, dataSize)) {
		return true;
	}
	return false;
}

bool Socket::peek(void *data, int dataSize) {
	readAll();
	if (buffer.peekBytes(data, dataSize)) {
		return true;
	}
	return false;
}

bool Socket::skip(int skipSize) {
	if (buffer.bytesAvailable() >= skipSize) {
		buffer -= skipSize;
		return true;
	}
	return false;
}

void Socket::setNoDelay() {
	int val = 1;
	int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&val, sizeof(int));
	if (result == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

void Socket::setBlock(bool block) {
#	ifdef USE_POSIX_SOCKETS
		int err = fcntl(sock, F_SETFL, block ? 0 : O_NONBLOCK);
#	else
		u_long iMode = !block;
		int err = ioctlsocket(sock, FIONBIO, &iMode);
#	endif
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

bool Socket::isReadable() {
	if (sock == INVALID_SOCKET) {
		return false;
	}
	TIMEVAL tv;
	tv.tv_sec= 0;
	tv.tv_usec= 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(sock + 1, &set, NULL, NULL, &tv);
	if (i == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
	return (i == 1);
}

bool Socket::isWritable() {
	if (sock == INVALID_SOCKET) {
		return false;
	}
	TIMEVAL tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i = select(sock + 1, NULL, &set, NULL, &tv);
	if (i == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
	return (i == 1);
}

bool Socket::isConnected() {
	//if the socket is not writable then it is not conencted
	if (!isWritable()) {
		return false;
	}
	//
	// commented out because we drain data into our own buffer, hence there
	// should generally not be any data available to read from the socket
	//
	//if the socket is readable it is connected if we can read a byte from it
	/*if (isReadable()) {
		char tmp;
		return (recv(sock, &tmp, sizeof(tmp), MSG_PEEK) > 0);
	}*/
	//otherwise the socket is connected
	return true;
}

string Socket::getHostName() const{
	const int strSize= 256;
	char hostname[strSize];
	gethostname(hostname, strSize);
	return hostname;
}

string Socket::getIp() const {
	hostent* info = gethostbyname(getHostName().c_str());
	if (!info) {
		handleError(__FUNCTION__);
	}
	unsigned char* address =
		reinterpret_cast<unsigned char*>(info->h_addr_list[0]);
	if (!address) {
		throw runtime_error("Error getting host ip");
	}
	return
		intToStr(address[0]) + "." +
		intToStr(address[1]) + "." +
		intToStr(address[2]) + "." +
		intToStr(address[3]);
}

void Socket::handleError(const char *caller) const {
	std::stringstream msg;
	int errCode = get_error();
	msg << "Socket Error in : " << caller << "() [Error code: " << errCode << "]\n";

#if defined(WIN32)
	LPVOID errMsg;
	DWORD msgRes = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER  | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errMsg, 0, NULL
	);
	if (msgRes) {
		msg << "\n" << (LPTSTR)errMsg;
		LocalFree((HLOCAL)errMsg);
	} else {
		msg << "\nFormatMessage() call failed. :~( [Error code: " << GetLastError() << "]";
	}
#else
	msg << strerror(errCode);
#endif
	cout << msg.str() << endl;
	throw SocketException(msg.str());
}

// =====================================================
//	class ClientSocket
// =====================================================

void ClientSocket::connect(const Ip &ip, int port) {
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip.getString().c_str());
	addr.sin_port = htons(port);

	int res = ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if (res == SOCKET_ERROR) {
		int errCode = get_error();
		if (errCode != WOULD_BLOCK) {
			handleError(__FUNCTION__);
		} else {

			// TODO
			// wait a bit, check writable, try again
			cout << "::connect() error, WOULD_BLOCK.\n";

		}
	} else {
		setNoDelay();
	}
}

Ip ClientSocket::receiveAnnounce(int port, char *message, int dataSize) {
	if (!udpBound) {
		udpBound = true;

		// bind
		struct sockaddr_in my_addr;
		my_addr.sin_family = AF_INET; // host byte order
		my_addr.sin_port = htons(port); // short, network byte order
		my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
		memset(&(my_addr.sin_zero), 0, 8); // zero the rest of the struct

		if (bind(udpsockfd,(struct sockaddr*)&my_addr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
			perror("bind error");
		}
	}

	struct sockaddr_in their_addr; // connector’s address information

	int addr_len = sizeof(struct sockaddr);
	int numbytes = recvfrom(udpsockfd, message, dataSize - 1, 0,
			(struct sockaddr*)&their_addr, (socklen_t*)&addr_len);
	if (numbytes == SOCKET_ERROR) {
		throw SocketException("exiting receive");
	}

	message[numbytes] = '\0';

	return Ip(inet_ntoa(their_addr.sin_addr));
}

void ClientSocket::disconnectUdp() {
    socket_close(udpsockfd);
}


// =====================================================
//	class ServerSocket
// =====================================================

void ServerSocket::bind(int port) {
	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= INADDR_ANY;
	addr.sin_port= htons(port);

	int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val));

	int err = ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

void ServerSocket::listen(int connectionQueueSize) {
	int err = ::listen(sock, connectionQueueSize);
	if (err == SOCKET_ERROR) {
		handleError(__FUNCTION__);
	}
}

Socket *ServerSocket::accept() {
	SOCKET newSock = ::accept(sock, NULL, NULL);
	if (newSock == INVALID_SOCKET) {
		if (get_error() == WOULD_BLOCK) {
			return NULL;
		}
		handleError(__FUNCTION__);
	}
	return new Socket(newSock);
}

void ServerSocket::sendAnnounce(int port) {
	struct sockaddr_in their_addr;
	their_addr.sin_family = AF_INET; //host byte order
	their_addr.sin_port = htons(port); //short, network byte order
	their_addr.sin_addr.s_addr = inet_addr(broadcastAddr.getString().c_str());
	memset(&(their_addr.sin_zero), 0, 8); //zero the rest of the struct

	string message = getHostName();

	int numbytes = sendto(udpsockfd, message.c_str(), strlen(message.c_str()), 0,
			(struct sockaddr*)&their_addr, sizeof(struct sockaddr));
	if (numbytes == SOCKET_ERROR) {
		throw SocketException("problem sending announce");
	}
}

}}//end namespace


