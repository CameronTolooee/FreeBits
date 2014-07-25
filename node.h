/*
 * node.h
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 * 
 * Header file for the node executable.
 * 
 */

#ifndef NODE_H_
#define NODE_H_
#include "wireformats.h"
#include "node_timer.h"
#include <map>
struct NodeFileInfo {
	uint16_t node_id;
 	struct in_addr ip;
	uint16_t port;
	char* downloaded_segments;
};

class Node {
public:
	Node(NodeConfigPacket pkt);
	virtual ~Node();
	void log(std::string str);
	void startFileTimers();
	void createNtTPacket(NtTPacket *ntt, std::vector<int> group);
	TestApp timerManager;
private:
	long getFileSize(std::string filename);
	void listen();
	void handlePacket(char * buf);
	void printGroupAssignments();
	bool hasFile(std::string);
	void broadcastIHave(std::string);
	std::vector<std::pair<int,NodeFileInfo>> choose8(std::string filename);
	void sendDownloadRequests(std::vector<std::pair<int,NodeFileInfo>> nodes, std::string filename);
	void createDLPacket(DownloadPacket *dlp, std::string filename, std::vector<int> seg_numbers, std::vector<std::array<char, 32>> seg_data);
	bool writeFileSegment(std::string filename, int segment, char *data);
	void createDLRPacket(DownloadRequestPacket *dlr, std::vector<uint16_t> seg, std::string filename);
	void createIHavePacket(IHavePacket * pkt, std::string filename, int v);

	/* map: filename --> vector[NodeInfo] */
	std::map<std::string, std::vector<NodeFileInfo>> group_assignments; 
	std::string name;
	time_t start;
	bool exitflag;
	int tracker_socket;
	int myTCPsocket;
	char myTCPport[5];
	NodeConfigPacket node_pkt;
	std::map<std::string, bool> completed_files;
	std::map<std::string, char *> node_files;
	std::map<std::string, int> dlcount;
	std::map<std::string, int> isent;
	std::map<std::string, int> timeout_count;
	std::map<std::string, int> igotback;
/* timer stuff */
protected:
	Timers *timersManager_;
public:
  	long ProcessTimer(int p);
	void addTimer(TimerCallback *tbc, unsigned short starttime);
};

struct my_bin_op : public std::binary_function <bool,bool,bool> {
  bool operator() (const bool& x, const bool& y) const { 
	std::cout<< "!"<<x<<"&&"<<y<<"="<<std::endl;
	return !x&&y;
	}

};

struct fd_state {
    char buffer[16384];
    size_t buffer_used;

    int writing;
    size_t n_written;
    size_t write_upto;
};


#endif /* NODE_H_ */
