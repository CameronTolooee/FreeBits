/*
 * tracker.cc
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 * 
 * Tracker executable - Maintains clients and interests. Acts as a central 
 * repository for clients to discover file locations and register their own 
 * information.
 *
 */

#include "tracker.h"
#include "connection.h"
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <netdb.h>

Tracker::Tracker(const char* manPort) {
	/* start up udp server and report back to manager with the port */
	udpSock = Connection::bindToPort("0", false);
	Connection::getPort(udpSock, port);
	if ((sockfd = Connection::connectToPort(manPort, true)) == -1) {
		perror("tracker connect");
	}
	std::ofstream log("tracker.out");
	log << "################################\n";
	log << " type: Tracker\n";
	log << "  pid: " << getpid() << "\n";
	log << "tPort: " << port << "\n";
	log << "################################\n";
	log << "ID\tTimeRecv\tFilename\n";
	log.close();
	if ((send(sockfd, port, sizeof port, 0)) == -1) {
		perror("tracker send");
	}
	time(&start_time);
}

Tracker::~Tracker() {
	// TODO Auto-generated destructor stub
}

void Tracker::handlePacket(NtTPacket *ntt, struct sockaddr_in addr) {
	/* 	1 = need group, no share
		2 = need group, share
		3 = no group, share 	*/
//	std::map<std::string, std::vector<NodeInfo>> file_groups; so I can see it >.<
	for (int i = 0; i < ntt->num_files; ++i) {		
		switch(ntt->sharetypes[i]) {
			case 1: {
				// make a group response pkt
				struct NodeInfo info = {ntt->node_id, addr.sin_addr, ntt->node_port};
				TtNPacket ttn;
				std::string f = ntt->filenames[i];
				createTTNPacket(&ttn, info, ntt, f);
				char buf[ttn.size()];
				ttn.hton(buf);
				// send it..
				int socket = Connection::connectToPort(ntt->node_port, true);
				log(ttn, ntt->node_id, buf);
				send(socket, buf, ttn.size(), 0);
				close(socket);
				break;
			}
			case 2:
			case 3: {
				struct NodeInfo info = {ntt->node_id, addr.sin_addr, ntt->node_port};
				bool flag = false;
				for(auto grp : file_groups[ntt->filenames[i]]){
					if(grp.node_id == ntt->node_id) {
						flag = true;
					}
				}
				if(!flag) {
					file_groups[ntt->filenames[i]].push_back(info);
				}
				if(ntt->sharetypes[i] == 2){
					// make a group response pkt
					TtNPacket ttn;
					std::string f = ntt->filenames[i];
					createTTNPacket(&ttn, info, ntt, f);
					char buf[ttn.size()];
					ttn.hton(buf);
					// send it..
					int socket = Connection::connectToPort(ntt->node_port, true);
					log(ttn, ntt->node_id, buf);
					send(socket, buf, ttn.size(), 0);
					close(socket);
				}
				break;
			}
			case 0: // no group, no shares --- do nothing
				break;	
					
			default: 
				std::cerr << "Tracker: packet type: "<< ntt->sharetypes[i] <<" not recognized." << std::endl;
		}
	}
}

void Tracker::listen(){
	struct sockaddr_in their_addr;
	int numbytes;
	char buf[1000];
	Connection::getPort(udpSock, port);

	struct timeval tv;
	tv.tv_sec = 10;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  
	setsockopt(udpSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	//int cnt = 0;
	while(true){
		unsigned int addr_len = sizeof(their_addr);
		if ((numbytes = recvfrom(udpSock, buf, 999 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			std::cout << "Tracker terminating." << std::endl;	
			close(udpSock);		
			exit(1);
		}
		NtTPacket ntt(buf);
		handlePacket(&ntt, their_addr);
		
	}
	close(udpSock);
}

void Tracker::createTTNPacket(TtNPacket *ttn, struct NodeInfo info, NtTPacket *pkt, std::string filename){
	ttn->msgtype = 2;
	ttn->num_files = pkt->num_files;
	ttn->filename = filename;
	ttn->num_neighbors = file_groups[filename].size();
	for(int i = 0; i < ttn->num_neighbors; ++i) {
		ttn->neighbor_ids.push_back(file_groups[filename][i].node_id);
		uint32_t u32;
		memcpy(&u32, &file_groups[filename][i].ip, 4);
		ttn->neighbor_ips.push_back(u32);
		ttn->neighbor_ports.push_back(file_groups[filename][i].port);
	}
}


void Tracker::log(TtNPacket pkt, uint16_t node_id, char* buf) {
	time_t now;
	time(&now);
	std::ofstream log("tracker.out", std::ios::app);
	log << node_id << "\t";
	log << "   " <<difftime(now, start_time) << "\t\t";
	log << pkt.filename << "\n";
	log <<"---- ";
	for(int i = 0; i < pkt.size(); ++i) {
		log << hex(buf[i]) << " ";
	} log << std::endl;
	log.close();
}

int main(int argc, const char*argv[]) {
	Tracker tracker(argv[0]);
	tracker.listen();
}
