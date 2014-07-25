/*
 * wireformats.h
 *
 *  Created on: Mar 20, 2014
 *      Author: ctolooee
 *
 * Defines all of the packets used in communication between entities in FreeBits
 * and how to serialize them for safe network i/o.
 *
 */

#ifndef WIREFORMATS_H_
#define WIREFORMATS_H_
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>

class NodeConfigPacket {
public:
	NodeConfigPacket(uint16_t timeout, uint16_t type, uint16_t node_id, uint16_t packet_delay,
			float drop_probability, uint16_t num_files, std::vector<std::string> filenames,
			uint16_t num_tasks, std::vector<std::string> task_files, 	std::vector<uint16_t>  starttimes,
				std::vector<uint16_t>  shares, uint16_t tport) : timeout(timeout),
			type(type), node_id(node_id), packet_delay(packet_delay), drop_probability(
					drop_probability), num_files(num_files), filenames(
					filenames), num_tasks(num_tasks), task_files(task_files), starttimes(
					starttimes), shares(shares), tport(tport) {
	}
	NodeConfigPacket() {
	}
	NodeConfigPacket(char* buf) {
		uint16_t u16;
		int offset = 0;
		memcpy(&u16, buf+offset, 2);
		offset += 2;
		timeout = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		type = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		node_id = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		packet_delay = ntohs(u16);
		memcpy(&drop_probability, buf+offset, sizeof(float));
		offset += sizeof(float);
		memcpy(&u16, buf+offset, 2);
		offset += 2;
		num_files = ntohs(u16);
		for(int i = 0; i < num_files; ++i) {
			char f[32];
			memcpy(f, buf+offset, 32);
			offset += 32;
			filenames.push_back(f);
		}
		memcpy(&u16, buf+offset, 2);
		offset += 2;
		num_tasks = ntohs(u16);
		for(int i = 0; i < num_tasks; ++i) {
			char d[32];
			memcpy(d, buf+offset, 32);
			offset += 32;
			task_files.push_back(d);
		}
		for(int i = 0; i < num_tasks; ++i) {
			memcpy(&u16, buf+offset, 2);
			offset += 2;
			starttimes.push_back(ntohs(u16));
		}
		for(int i = 0; i < num_tasks; ++i) {
			memcpy(&u16, buf+offset, 2);
			offset += 2;
			shares.push_back(ntohs(u16));
		}
		memcpy(&u16, buf+offset, 2);
		offset += 2;
		tport = ntohs(u16);
	}
	~NodeConfigPacket() {
	}
	int size() {
		return 14 + sizeof(float) + (32 * num_files) + (32 * num_tasks)
				+ (2 * num_tasks) + (2 * num_tasks);
	}
	char* hton(char *buf) {
		uint16_t u16 = htons(timeout);
		int offset = 0;
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		u16 = htons(type);
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		u16 = htons(node_id);
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		u16 = htons(packet_delay);
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		memcpy(buf+offset, &drop_probability, sizeof(float));
		offset += sizeof(float);

		u16 = htons(num_files);
		memcpy(buf+offset, &u16, 2);
		offset += 2;
		for(int i = 0; i < num_files; ++i) {
			char f[32];
			strcpy(f, filenames[i].c_str());
			memcpy(buf+offset, &f, 32);
			offset += 32;
		}
		u16 = htons(num_tasks);
		memcpy(buf+offset, &u16, 2);
		offset+=2;
		for(int i = 0; i < num_tasks; ++i) {
			char d[32];
			strcpy(d, task_files[i].c_str());
			memcpy(buf+offset, &d, 32);
			offset += 32;
		}
		for(int i = 0; i < num_tasks; ++i) {
			u16 = htons(starttimes[i]);
			memcpy(buf+offset, &u16, 2);
			offset += 2;
		}
		for(int i = 0; i < num_tasks; ++i) {
			u16 = htons(shares[i]);
			memcpy(buf+offset, &u16, 2);
			offset += 2;
		}
		u16 = htons(tport);
		memcpy(buf+offset, &u16, 2);
		offset += 2;
		return buf;
	}
	uint16_t timeout = 0;
	uint16_t type = 0;
	uint16_t node_id = 0;
	uint16_t packet_delay = 0;
	float drop_probability = 0;
	uint16_t num_files = 0;
	std::vector<std::string> filenames;
	uint16_t num_tasks = 0;
	std::vector<std::string> task_files;
	std::vector<uint16_t> starttimes;
	std::vector<uint16_t> shares;
	uint16_t tport = 0;
};

class NtTPacket {
public:
	NtTPacket(uint16_t msgtype, uint16_t node_id, uint16_t node_port, uint16_t num_files, 
		std::vector<std::string> filenames, std::vector<uint16_t> sharetypes) :
		msgtype(msgtype), node_id(node_id), node_port(node_port),num_files(num_files), filenames(filenames),
		sharetypes(sharetypes) { }
	NtTPacket() { }
	NtTPacket(char *buf) { 
		uint16_t u16;
		int offset = 0;
		memcpy(&u16, buf, 2);
		offset += 2;
		msgtype = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		node_id = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		node_port = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		num_files = ntohs(u16);

		for(int i = 0; i < num_files; ++i) {		
			char d[32];
			memcpy(d, buf+offset, 32);
			offset += 32;
			filenames.push_back(d);		
		}
		for(int i = 0; i < num_files; ++i) {		
			memcpy(&u16, buf+offset, 2);
			offset += 2;
			sharetypes.push_back(ntohs(u16));		
		}
	}
	~NtTPacket() {}
	void hton(char * buf) {
		uint16_t u16 = htons(msgtype);
		int offset = 0;
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		u16 = htons(node_id);
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		u16 = htons(node_port);
		memcpy(buf+offset, &u16, 2);
		offset += 2;	
	
		u16 = htons(num_files);
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		for(int i = 0; i < num_files; ++i) {
			char f[32];
			strcpy(f, filenames[i].c_str());
			memcpy(buf+offset, &f, 32);
			offset += 32;
		}
		for(int i = 0; i < num_files; ++i) {
			u16 = htons(sharetypes[i]);
			memcpy(buf+offset, &u16, 2);
			offset += 2;		
		}
	}
	int size() {
		return 8 + (num_files * 32) + (num_files * 2);
	}
	uint16_t msgtype = 0;
	uint16_t node_id = 0;
	uint16_t node_port = 0;
	uint16_t num_files = 0;
	std::vector<std::string> filenames;
	std::vector<uint16_t> sharetypes;
};

class TtNPacket {
public:
	TtNPacket(uint16_t msgtype, uint16_t num_files, std::string filename,
		uint16_t num_neighbors, std::vector<uint16_t> neighbor_ids, 
		std::vector<uint32_t> neighbor_ips, std::vector<uint16_t> neighbor_ports) :
		msgtype(msgtype), num_files(num_files), filename(filename),
		num_neighbors(num_neighbors), neighbor_ids(neighbor_ids), 
		neighbor_ips(neighbor_ips), neighbor_ports(neighbor_ports) { }
	TtNPacket () {}
	TtNPacket (char*  buf) {
		uint16_t u16;
		uint32_t u32;
		int offset = 0;

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		msgtype = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		num_files = ntohs(u16);	
		
		char d[32];
		memcpy(d, buf+offset, 32);
		offset += 32;
		filename = d;

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		num_neighbors = ntohs(u16);		
		for(int i = 0; i < num_neighbors; ++i) {
			memcpy(&u16, buf+offset, 2);
			offset += 2;
			neighbor_ids.push_back(ntohs(u16));
		}
		for(int i = 0; i < num_neighbors; ++i) {
			memcpy(&u32, buf+offset, 4);
			offset += 4;
			neighbor_ips.push_back(u32);
		}
		for(int i = 0; i < num_neighbors; ++i) {
			memcpy(&u16, buf+offset, 2);
			offset += 2;
			neighbor_ports.push_back(ntohs(u16));
		}
	}
	~TtNPacket() {}
	void hton(char * buf){
		uint16_t u16 = htons(msgtype);
		int offset = 0;
		memcpy(buf+offset, &u16, 2);
		offset += 2;
	
		u16 = htons(num_files);
		memcpy(buf+offset, &u16, 2);
		offset += 2;		
	
		char f[32];
		strcpy(f, filename.c_str());
		memcpy(buf+offset, &f, 32);
		offset += 32;

		u16 = htons(num_neighbors);
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		for(int i = 0; i < num_neighbors; ++i) {
			u16 = htons(neighbor_ids[i]);
			memcpy(buf+offset, &u16, 2);
			offset += 2;				
		}
		uint32_t u32;
		for(int i = 0; i < num_neighbors; ++i) {
			u32 = neighbor_ips[i];
			memcpy(buf+offset, &u32, 4);
			offset += 4;				
		}
		for(int i = 0; i < num_neighbors; ++i) {
			u16 = htons(neighbor_ports[i]);
			memcpy(buf+offset, &u16, 2);
			offset += 2;				
		}
	}
	int size() {
		return 38 + (num_neighbors * 2) + (num_neighbors * 2) + (num_neighbors * 4) ;
	}
	uint16_t msgtype = 0;
	uint16_t num_files = 0;
	std::string filename;
	uint16_t num_neighbors = 0;
	std::vector<uint16_t> neighbor_ids;
	std::vector<uint32_t> neighbor_ips;
	std::vector<uint16_t> neighbor_ports;
};

class IHavePacket {
public:
	IHavePacket(){};
	IHavePacket(uint16_t msgtype, uint16_t node_id, uint16_t port, std::string filename, uint16_t total_segments,
		char * ds) : msgtype(msgtype), node_id(node_id), port(port),
		 filename(filename), total_segments(total_segments){ downloaded_segments = (char*)malloc(81*sizeof(char));};
	~IHavePacket(){};
	IHavePacket(char* buf) {
		uint16_t u16;
		char d[32];
		int offset = 0;
		memcpy(&u16, buf+offset, 2);
		offset += 2;
		msgtype = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		node_id = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		port = ntohs(u16);

		memcpy(d, buf+offset, 32);
		offset += 32;
		filename = d;

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		total_segments = ntohs(u16);

		downloaded_segments = (char*)malloc(81*sizeof(char));
		memcpy(downloaded_segments, buf+offset, (total_segments /8)+1);
	}

	void hton(char* buf) {
		uint16_t u16;
		char d[32];
		int offset = 0;

		u16 = htons(msgtype);
		memcpy(buf+offset, &u16, 2);
		offset += 2;

		u16 = htons(node_id);
		memcpy(buf+offset, &u16, 2);
		offset += 2;			

		u16 = htons(port);
		memcpy(buf+offset, &u16, 2);
		offset += 2;		
	
		strcpy(d, filename.c_str());
		memcpy(buf+offset, &d, 32);
		offset += 32;

		u16 = htons(total_segments);
		memcpy(buf+offset, &u16, 2);
		offset += 2;
			
		memcpy(buf+offset, downloaded_segments, total_segments);
		offset += u16;
	}

	int size() {
		return 40 + total_segments;
	}
	uint16_t msgtype = 0;
	uint16_t node_id =0;
	uint16_t port = 0;
	std::string filename;
	uint16_t total_segments = 0;
	char *downloaded_segments;
};

class DownloadPacket {
public:
	DownloadPacket(uint16_t msgtype, uint16_t node_id, std::string filename, uint16_t total_segments,
		std::vector<int> seg_numbers, std::vector<std::array<char,32>> seg_data): msgtype(msgtype), node_id(node_id), filename(filename),
		seg_numbers(seg_numbers), seg_data(seg_data){ }
	DownloadPacket(){}
	~DownloadPacket(){}
	DownloadPacket(char* buf) {
		uint16_t u16;
		char d[32];
		int offset = 0;

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		msgtype = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		node_id = ntohs(u16);

		memcpy(d, buf+offset, 32);
		offset += 32;
		filename = d;

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		int num = ntohs(u16);		

		for(int i = 0; i < num; ++i) {
			memcpy(&u16, buf+offset, 2);
			offset += 2;
			seg_numbers.push_back(ntohs(u16));
		}

		seg_data = std::vector<std::array<char,32>>(num);
		for(int i = 0; i < num; ++i) {
			memcpy(seg_data[i].data(), buf+offset, 32);
			offset += 32;
		}
	}
	void hton(char* buf) {
		uint16_t u16;
		char d[32];
		int offset = 0;

		u16 = htons(msgtype);
		memcpy(buf+offset, &u16, 2);
		offset += 2;		

		u16 = htons(node_id);
		memcpy(buf+offset, &u16, 2);
		offset += 2;	
	
		strcpy(d, filename.c_str());
		memcpy(buf+offset, &d, 32);
		offset += 32;

		u16 = htons(seg_numbers.size());
		memcpy(buf+offset, &u16, 2);
		offset += 2;	

		for(auto i : seg_numbers) {
			u16 = htons(i);
			memcpy(buf+offset, &u16, 2);
			offset += 2;	
		}

		for(auto i : seg_data) {
			memcpy(buf+offset, i.data(), 32);
			offset += 32;
		}

	}

	int size() {
		return 38 +(seg_numbers.size() * 2) +(seg_numbers.size() * 32);
	}
	uint16_t msgtype = 6;
	uint16_t node_id;
	std::string filename;
	std::vector<int> seg_numbers;
	std::vector<std::array<char,32>> seg_data;
};

class DownloadRequestPacket {
public:
	DownloadRequestPacket(uint16_t msgtype, uint16_t node_id, uint16_t port, std::string filename, uint16_t num_seg,
		std::vector<uint16_t> segments) : msgtype(msgtype), node_id(node_id), 
		port(port), filename(filename), num_seg(num_seg), segments(segments){ }
	
	~DownloadRequestPacket() {}
	DownloadRequestPacket() {}
	DownloadRequestPacket(char * buf) {
		uint16_t u16;
		int offset = 0;
		char d[32];
		memcpy(&u16, buf+offset, 2);
		offset += 2;
		msgtype = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		node_id = ntohs(u16);

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		port = ntohs(u16);

		memcpy(d, buf+offset, 32);
		offset += 32;
		filename = d;

		memcpy(&u16, buf+offset, 2);
		offset += 2;
		num_seg = ntohs(u16);

		for(int i = 0; i < num_seg; ++i) {
			memcpy(&u16, buf+offset, 2);
			offset += 2;
			segments.push_back(ntohs(u16));
		}

	}

	void hton(char* buf) {
		uint16_t u16;
		int offset = 0;
		char d[32];

		u16 = htons(msgtype);
		memcpy(buf+offset, &u16, 2);
		offset += 2;	

		u16 = htons(node_id);
		memcpy(buf+offset, &u16, 2);
		offset += 2;	

		u16 = htons(port);
		memcpy(buf+offset, &u16, 2);
		offset += 2;	

		strcpy(d, filename.c_str());
		memcpy(buf+offset, &d, 32);
		offset += 32;

		u16 = htons(num_seg);
		memcpy(buf+offset, &u16, 2);
		offset += 2;	

		for(int i = 0; i < num_seg; ++i){
			u16 = htons(segments[i]);
			memcpy(buf+offset, &u16, 2);
			offset += 2;
		}
	}

	int size(){
		return 40 + (2 * num_seg);
	}

	uint16_t msgtype = 5;
	uint16_t node_id = 0;
	uint16_t port = 0;
	std::string filename;	
	uint16_t num_seg = 0;
	std::vector<uint16_t> segments;
};


struct NodeInfo {
	uint16_t node_id;
 	struct in_addr ip; //subject to change
	uint16_t port;
};	
#endif /* WIREFORMATS_H_ */
