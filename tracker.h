/*
 * tracker.h
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 *
 * Header file for the Tracker executable.
 *
 */

#ifndef TRACKER_H_
#define TRACKER_H_
#include "wireformats.h"
#include <map>
#include <ctime>

class Tracker {
public:
	Tracker(const char * manPort);
	virtual ~Tracker();
	void listen();	
private:
	void createTTNPacket(TtNPacket *ttn, struct NodeInfo info, NtTPacket *pkt, std::string filename);
	void handlePacket(NtTPacket *ntt, struct sockaddr_in addr);
	void log(TtNPacket pkt, uint16_t node_id, char * buf);


	std::map<std::string, std::vector<NodeInfo>> file_groups;
	int udpSock = 0, sockfd = 0;
	char port[5];
	time_t start_time;
	
};

#endif /* TRACKER_H_ */
