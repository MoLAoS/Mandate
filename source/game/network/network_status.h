// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================


#ifndef _GLEST_GAME_NETWORKSTATUS_H_
#define _GLEST_GAME_NETWORKSTATUS_H_

#include <string>
#include <vector>
#include <queue>
#include <string>

#include "timer.h"
#include "types.h"

using Shared::Platform::Chrono;
using Shared::Platform::int64;
using namespace std;

namespace Glest { namespace Game {

class NetworkStatus {
public:
	class DataInfo {		
		class Item {
			int64 time;
			size_t size;
	
		public:
			Item(size_t size) : time(Chrono::getCurMicros()), size(size) {}
			// allow default copy ctor
			int64 getTime() const					{return time;}
			size_t getSize() const					{return size;}
			bool isOlderThan(int64 refTime) const	{return time < refTime;}
		};
		
		int64 maxAge;
		queue<Item> q;
		int64 lastCleaned;
		size_t size;

	public:
		DataInfo(int64 maxAge) : maxAge(maxAge), q(), lastCleaned(-1), size(0) {}
		size_t getSize() const	{return size;}

		void add(size_t size) {
			q.push(Item(size));
			this->size += size;
		}

		void clean() {
			lastCleaned = Chrono::getCurMicros();
			int64 refTime = lastCleaned - maxAge;
			while(!q.empty() && q.front().isOlderThan(refTime)) {
				size -= q.front().getSize();
				q.pop();
			}
		}

		float getBytesPerSecond() const {
			int64 now = Chrono::getCurMicros();
			int64 age = now - lastCleaned + maxAge;
			return (float)size / (float)age * 1000000.f;
		}
	};

private:
	DataInfo dataSent;
	DataInfo dataRecieved;
	float txBytesPerSecond;
	float rxBytesPerSecond;
	int64 updateInterval;
	int64 lastUpdate;
	int64 latency;
	int64 avgLatency;
	string statusStr;
	deque<int64> pingTimes;
	//deque<int64> timeDiffs;

	static const int pingHistorySize = 10;

public:
	NetworkStatus(int64 updateInterval = 1000000, int64 throughputHistory = 5000000) :
			dataSent(throughputHistory),
			dataRecieved(throughputHistory),
			updateInterval(updateInterval),
			lastUpdate(-1),
			latency(-1) {
	}

	int64 getLastUpdate() const					{return lastUpdate;}
	int64 getLatency() const					{return latency;}
	int64 getAvgLatency() const					{return avgLatency;}
	void addDataSent(size_t bytes)				{dataSent.add(bytes);}
	void addDataRecieved(size_t bytes)			{dataRecieved.add(bytes);}
	virtual string getStatus() const			{return statusStr;}
	virtual bool isConnected() = 0;
	virtual void update(); 

protected:
	virtual void ping() = 0;
	void pong(int64 departureTime, int64 remoteTime, int64 arrivalTime = Chrono::getCurMicros());

private:
	void updateStatusStr();
};

}}//end namespace

#endif