/*
 * manager.cc
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 *
 * Manager executable - Reads the manager.conf file and spawns the specified 
 * nodes and tracker to initialize the FreeBits system and begin execution.
 *
 */

#include "manager.h"
#include "connection.h"
#include "wireformats.h"
#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

Manager::Manager(std::string file) :
		file(file) {
	std::cout << "Manager starting up tracker and nodes." << std::endl;
}

Manager::~Manager() {
}

void Manager::forkProcesses(char * server_port, ConfFileInfo file_info) {
	int pid[file_info.num_nodes];
	for (int i = 0; i < file_info.num_nodes; ++i) {
		if ((pid[i] = fork()) < 0) {
			exit(1);
		} else if (pid[i] == 0) { // CHILD
			execl("node", server_port, NULL);
			printf("ERROR #%d: %s\n", errno, strerror(errno));
			break;
		} else { // PARENT

		}
	}
}

void Manager::createConfigurations(ConfFileInfo info,
		std::vector<NodeConfigPacket*> &configs, uint16_t tport) {
	for (int i = 0; i < info.num_nodes; ++i) {
		uint16_t node_id;
		uint16_t packet_delay;
		float drop_probability;
		std::vector<std::string> filenames;
		std::vector<std::string> task_files;
		std::vector<uint16_t>  stimes;
		std::vector<uint16_t>  v_shares;
		node_id = i;
		packet_delay = info.packet_info[i].delay;
		drop_probability = info.packet_info[i].packet_drop_prob;
		for (unsigned int j = 0; j < info.file_locations.size(); ++j) {
			if (info.file_locations[j].node_id == node_id) {
				filenames.push_back(info.file_locations[j].filename);
			}
		}
		for (unsigned int j = 0; j < info.download_tasks.size(); ++j) {
			if (info.download_tasks[j].node_id == node_id) {
				task_files.push_back(info.download_tasks[j].filename);
				stimes.push_back(info.download_tasks[j].start_time);
				v_shares.push_back(info.download_tasks[j].share);
			}
		}
		configs.push_back(new NodeConfigPacket (info.timeout, 0, node_id, packet_delay, drop_probability,
				(uint16_t) filenames.size(), filenames, (uint16_t) stimes.size(), task_files,
				stimes, v_shares, tport));
	}
}

#define MAN_MAIN_
#ifdef MAN_MAIN_
int main() {
	char port[5];
	Manager man("manager.conf");
	int sockfd = Connection::bindToPort("0", true);
	Connection::getPort(sockfd, port);

	/* listen for incoming connect()'s (this does not block); */
	if (listen(sockfd, 25) == -1) {
		perror("manager listen");
		exit(1);
	}

	ConfFileParser parser("manager.conf");
	ConfFileInfo info;
	try {
		info = parser.readFile();
	} catch(char const* j) { 	std::cerr << j<< std::endl; }
	std::vector<NodeConfigPacket*> configs;
	/* create child tracker process */
	int tracker_pid;
	if ((tracker_pid = fork()) < 0) {
		exit(1);
	} else if (tracker_pid == 0) { /* child -- exec the tracker and give it the port we're listening on */
		execl("tracker", port, NULL);
	} else {
		int tracker = Connection::acceptConnection(sockfd);
		int num_bytes;
		char buf[100];
		num_bytes = recv(tracker, buf, 99, 0);
		if (num_bytes == -1) {
			perror("recv");
		}
		buf[num_bytes] = '\0';
		uint16_t tport = atoi(buf);
		man.createConfigurations(info, configs, tport);
		man.forkProcesses(port, info);
		for (int i = 0; i < info.num_nodes; ++i) {
			int node = Connection::acceptConnection(sockfd);
			int num_bytes;
			char buf[100];
			num_bytes = recv(node, buf, 99, 0);
			if (num_bytes == -1) {
				perror("recv");
			}
			buf[num_bytes] = '\0';
			//printf("manager: received '%s'\n", buf);
			int pkt_size = configs[i]->size();
			char packet[pkt_size];
			configs[i]->hton(packet);
			long s = htonl(pkt_size);
			send(node, &s, sizeof(long), 0);
			send(node, &packet, s, 0);
		}
	}
	std::cout << "Manager terminating." << std::endl;
}
#endif
