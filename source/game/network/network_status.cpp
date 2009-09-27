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

#include "pch.h"
#include "network_status.h"
#include <sstream>
#include "math_util.h"

#include "leak_dumper.h"

using namespace std;

namespace Glest { namespace Game {

// =====================================================
//	class NetworkStatus
// =====================================================

void NetworkStatus::pong(int64 departureTime, int64 remoteTime, int64 arrivalTime) {
	latency = (arrivalTime - departureTime);
	pingTimes.push_back(latency);
	while(pingTimes.size() > pingHistorySize) {
		pingTimes.pop_front();
	}
	int64 totalPingTime = 0;
	for(deque<int64>::const_iterator i = pingTimes.begin(); i != pingTimes.end(); ++i) {
		totalPingTime += *i;
	}
	avgLatency = totalPingTime / pingTimes.size();
	//timeDiffs.push_back(remoteTime - departureTime - latency / 2);
	updateStatusStr();
}

static inline void prettyBytes(stringstream &str, float bytes) {
	if(bytes < 1024.f) {
		str << (size_t)roundf(bytes);
	} else {
		str << roundf(bytes / 100.f) / 10.f << "k";
	}
}

void NetworkStatus::updateStatusStr() {
	stringstream str;
	str << (latency / 1000) << " (" << (avgLatency / 1000) << ") Tx/Rx ";
	prettyBytes(str, txBytesPerSecond);
	str << "/";
	prettyBytes(str, rxBytesPerSecond);
	statusStr = str.str();
}

void NetworkStatus::update() {
	int64 curTime = Chrono::getCurMicros();
	if(curTime >= lastUpdate + updateInterval) {
		lastUpdate = curTime;
		ping();
		dataSent.clean();
		dataRecieved.clean();
		txBytesPerSecond = dataSent.getBytesPerSecond();
		rxBytesPerSecond = dataRecieved.getBytesPerSecond();
		updateStatusStr();
	}
}

}}//end namespace
