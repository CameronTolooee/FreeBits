/*
 * node.cc
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 *
 * Client executable - does the bulk of the work, responsible for: discovering
 * file locations, broacasting their own files, downloaded specifed files, and
 * terminating once complete.
 *
 */

#include "node.h"
#include "connection.h"
#include <unistd.h>
#include <fcntl.h>
#include <set>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/socket.h>

Node::Node(NodeConfigPacket pkt) {
	std::cout << "Node " << pkt.node_id << " Started." << std::endl;
	exitflag = false;
	node_pkt = pkt;
	myTCPsocket = Connection::bindToPort(0, true);
	Connection::getPort(myTCPsocket, myTCPport);

	std::stringstream sstream;
	sstream << pkt.node_id << ".out";
	name = sstream.str();
	std::ofstream log(name.c_str());
	log << "################################\n";
	log << "  type: Client\n";
	log << "  myID: " << pkt.node_id << "\n";
	log << "   pid: " << getpid() << "\n";
	log << " tPort: " << pkt.tport << "\n";
	log << "myPort: " << myTCPport << "\n";
	log << "################################\n";
	log << "Time\tFrom/To\t    MsgType\t\tMsgData\n";
	log.close();
	timersManager_ = new Timers();
	for(auto file : pkt.filenames) {
		int size = getFileSize(file);
		int num_seg = size/32;
		if(size % 32 != 0) {
			++num_seg;		
		}
		int seg_to_char = num_seg/8;
		if(num_seg % 8 != 0)
			++seg_to_char;
		char *bits = (char*)malloc(seg_to_char*sizeof(char));
		memset(bits, 0, seg_to_char);
		for(int k = 0; k < num_seg; ++k) {
			bits[k/8] |= 1 << (k%8);
		}
		node_files.insert(std::pair<std::string, char *>(file, bits));
		completed_files[file] = true;
	}
	int i = 0;
	for(auto file : pkt.task_files) {
		if(!hasFile(file)) {
			int size = getFileSize(file);
			int num_seg = size/32;
			if(size % 32 != 0) {
				++num_seg;		
			}
			int seg_to_char = num_seg/8;
			if(num_seg % 8 != 0)
				++seg_to_char;
			char *bits = (char*)malloc(seg_to_char*sizeof(char));
			memset(bits, 0, seg_to_char);
			node_files.insert(std::pair<std::string, char *>(file, bits));
			completed_files[file] = false;
		}
		++i;
	}
	time(&start);
	listen();
}

Node::~Node() {
}

void Node::startFileTimers() {
	std::map<int, std::vector<int>> groups; // starttimes --> index
	int cntr = 0;
	for(auto i : node_pkt.starttimes) {
		groups[i].push_back(cntr++);
	}
	for(auto i : groups) {
		NtTPacket ntt;
		createNtTPacket(&ntt, i.second);
		NodeTimer *n = new NodeTimer(node_pkt.node_id, ntt, node_pkt.tport, start);
		addTimer(n, i.first);
	}
}

void makeNonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

struct fd_state * alloc_fd_state(void) {
    struct fd_state *state = (fd_state*)malloc(sizeof(struct fd_state));
    if (!state)
        return NULL;
    state->buffer_used = state->n_written = state->writing = state->write_upto = 0;
    return state;
}

void Node::broadcastIHave(std::string filename) {
	for (auto node_info : group_assignments[filename]) {
		/* don't broadcast to self */
		if(node_info.node_id != node_pkt.node_id) {
			IHavePacket pkt;
			createIHavePacket(&pkt, filename, 3);
			char buf[pkt.size()];
			pkt.hton(buf);
			int sockfd = Connection::connectToPort(node_info.port, true);
			std::stringstream ss;
			time_t now;
			time(&now);
			ss << difftime(now, start) << "\tTO   "<< node_info.node_id <<"\tCLNT INFO REQ\t\t";
			ss << "/csu/cs557/" <<filename << "\n";
			ss.flush();
			usleep(node_pkt.packet_delay*1000);
			log(ss.str());
			if(send(sockfd, buf, pkt.size(), 0) == -1) {
				std::stringstream ss;
				ss << node_pkt.node_id << " ihave broadcast";
				perror(ss.str().c_str());
			}
			isent[filename]++;
			close(sockfd);
		}
	}
}

void Node::sendDownloadRequests(std::vector<std::pair<int,NodeFileInfo>> nodes, std::string filename) {
	// map  nodeid --> vector<segment numbers>
	std::map<int, std::vector<std::pair<int,NodeFileInfo>>> mapper;
	for(auto info : nodes) {
		mapper[info.second.node_id].push_back(info);
	}

	for(auto info : mapper) {
		std::vector<uint16_t> nums;
		for(auto i : info.second) {
			nums.push_back(i.first);			
		}
		DownloadRequestPacket dlr;
		createDLRPacket(&dlr, nums, filename);
		char buf[dlr.size()];
		dlr.hton(buf);
		int sockfd = Connection::connectToPort(info.second[0].second.port, true);
		usleep(node_pkt.packet_delay*1000);	
		std::stringstream ss;
		time_t now;
		time(&now);
		ss << difftime(now, start) << "\tTO   "<< info.first <<"\tCLNT SEG REQ\t\t";
		ss <<   "/csu/cs557/" << filename << " ";
		for(auto i : nums) {
			ss <<i << " ";
		} ss << "\n";
		ss.flush();
		log(ss.str());
		if(send(sockfd, buf, dlr.size(), 0) == -1) {
			perror("DLR send");
		}
		close(sockfd);
	}
}

void Node::handlePacket(char * buf) {
	uint16_t u16;
	memcpy(&u16, buf, 2);
	int msgtype = ntohs(u16);

	/* 2 = Tracker to Node -- group assignment
	 * 3 = Node to Node    -- I-Have message
	 * 4 = Node to Node    -- I-Have response
     * 5 = Node to Node    -- Give me message
     * 6 = Node to Node    -- Data download
	 */
	switch(msgtype) {
		case 2: {
	 		TtNPacket ttn(buf);
			if(completed_files[ttn.filename])
				return;
			time_t now;
			time(&now);
			std::stringstream ss;
			ss << difftime(now, start) << "\tFROM T\tGROUP_ASSIGNMENT\t";
			for(auto i : ttn.neighbor_ids) {
				if (i != node_pkt.node_id) {
					ss << i << " ";
				}				
			} ss << "\n";
			ss.flush();
			log(ss.str());
			bool flag = true;
			for(auto i : ttn.neighbor_ids){
				if(i != node_pkt.node_id) {
					flag = false;
				}
			}
			if(flag) {
				if(timeout_count[ttn.filename]++ > 2) {
					std::cerr << "Node "<< node_pkt.node_id <<" unable retrieve file "<< ttn.filename << ". Giving up..." << std::endl;
					return;
				}
				NtTPacket ntt;
				int loc = find(node_pkt.task_files.begin(), node_pkt.task_files.end(), ttn.filename) - node_pkt.task_files.begin();
				std::vector<int> tmp; tmp.push_back(loc);
				createNtTPacket(&ntt, tmp);
				NodeTimer *n = new NodeTimer(node_pkt.node_id, ntt, node_pkt.tport, start);
				addTimer(n, node_pkt.timeout);
				return;
			}
			for (unsigned int i = 0; i < ttn.num_neighbors; ++i) {
				if(ttn.neighbor_ids[i] == node_pkt.node_id) {
					continue;
				}
				char *bits = (char*)malloc(80*sizeof(char));
				memset(bits, 0, 80);
				NodeFileInfo info = {ttn.neighbor_ids[i], ttn.neighbor_ips[i], ttn.neighbor_ports[i], bits};
				bool flag = false;
				for(auto grp : group_assignments[ttn.filename]){
					if(grp.node_id == info.node_id) {
						flag = true;
					}
				}
				if(!flag) {
					group_assignments[ttn.filename].push_back(info);
				}
			}
			//printGroupAssignments();
			// Initiate IHave b-cast
			dlcount[ttn.filename] = 0;
			broadcastIHave(ttn.filename);
			// after num_neighbor responses, Give me bcast;
			break; 
		}
		case 3: {
			IHavePacket pkt(buf);
			time_t now;
			time(&now);
			std::stringstream ss;
			ss << difftime(now, start) << "\tFROM "<< pkt.node_id<<"\tCLNT INFO REQ\t\t";
			ss << "/csu/cs557/" << pkt.filename <<"\n";
			ss.flush();
			log(ss.str());
			ss.str(std::string()); //clear ss
			IHavePacket pkt2;
			createIHavePacket(&pkt2, pkt.filename, 4);
			char buf2[pkt2.size()];
			memset(buf2, 0, pkt2.size());
			pkt2.hton(buf2);
			int sockfd = Connection::connectToPort(pkt.port, true);
			time(&now);
			ss << difftime(now, start) << "\tTO   "<< pkt.node_id <<"\tCLNT INFO REP\t\t";
			ss <<   "/csu/cs557/" << pkt.filename << " ";
			for(int i = 0; i < pkt2.total_segments; ++i) {
				if((pkt2.downloaded_segments[i/8]	& (1 << (i%8))) != 0) {
					ss <<i << " ";
				}
			} ss << "\n";
			ss.flush();
			usleep(node_pkt.packet_delay*1000);
			log(ss.str());
			if(send(sockfd, buf2, pkt2.size(), 0) == -1) {
				perror("i-have response");
			}
			close(sockfd);
 			break; 
		}
		case 4: {

			IHavePacket pkt(buf);
			igotback[pkt.filename]++;

			int val = (pkt.total_segments + 8 - 1)/8;
			for (auto node_info : group_assignments[pkt.filename]) {
				if(node_info.node_id == pkt.node_id) {
					memcpy(node_info.downloaded_segments, pkt.downloaded_segments, val);
				}
			}
			time_t now;
			time(&now);
			std::stringstream ss;
			ss << difftime(now, start) << "\tFROM "<< pkt.node_id<<"\tCLNT INFO REP\t\t";
			ss << "/csu/cs557/" << pkt.filename <<" ";
			for(int i = 0; i < pkt.total_segments; ++i) {
				if((pkt.downloaded_segments[i/8] & (1 << (i%8))) != 0) {
					ss <<i << " ";
				}
			} ss << "\n";
			ss.flush();
			log(ss.str());
			if(isent[pkt.filename] == igotback[pkt.filename]) {	
				isent[pkt.filename] = 0;
				igotback[pkt.filename] = 0;
				std::vector<std::pair<int,NodeFileInfo>> nodes = choose8(pkt.filename);
				if(nodes.size() == 0) {
					if(timeout_count[pkt.filename]++ > 3) {
						std::cerr << "Node "<< node_pkt.node_id <<" unable retrieve file "<< pkt.filename << ". Giving up..." << std::endl;
						return;
					}
					NtTPacket ntt;
					int loc = find(node_pkt.task_files.begin(), node_pkt.task_files.end(), pkt.filename) - node_pkt.task_files.begin();
					std::vector<int> tmp; tmp.push_back(loc);
					createNtTPacket(&ntt, tmp);
					NodeTimer *n = new NodeTimer(node_pkt.node_id, ntt, node_pkt.tport, start);
					addTimer(n, node_pkt.timeout);
				}
				sendDownloadRequests(nodes, pkt.filename);
			}
			break;
		}
		case 5: {
			DownloadRequestPacket dlr(buf);
			time_t now;
			time(&now);
			std::stringstream ss;
			ss << difftime(now, start) << "\tFROM "<< dlr.node_id<<"\tCLNT SEG REQ\t\t";
			ss << "/csu/cs557/" << dlr.filename << " ";
			for(auto i : dlr.segments) {
				ss << i << " ";
			} ss << "\n";
			ss.flush();
			log(ss.str());
			ss.str(std::string()); // clear ss
			std::vector<int> seg_numbers;
			std::vector<std::array<char, 32>> seg_data;
			time(&now);
			ss << difftime(now, start) << "\tTO   "<< dlr.node_id<<"\tCLNT SEG REP\t\t"<< "/csu/cs557/" << dlr.filename ;
			for(auto i : dlr.segments) {
				int pos = i * 32;
				char d[32];
				memset(d, 0, 32);
				std::array<char, 32> *dat = new std::array<char,32>();
				std::ifstream is (dlr.filename.c_str(), std::ifstream::binary);
				is.seekg(pos);
				is.read(dat->data(), 32);
				seg_numbers.push_back(i);
				seg_data.push_back(*dat);				
				ss << " " << i;
			}
			ss << "\n";
			DownloadPacket dlp;
			createDLPacket(&dlp, dlr.filename, seg_numbers, seg_data);
			char data[dlp.size()];
			dlp.hton(data);
			int sockfd = Connection::connectToPort(dlr.port, true);
			ss.flush();
			usleep(node_pkt.packet_delay*1000);
			log(ss.str());
			if(send(sockfd, data, dlp.size(), 0) == -1) {
				perror("DL reponse");
			}
			close(sockfd);
 			break; 
		}
		case 6: {
			DownloadPacket dlp(buf);
			time_t now;
			time(&now);
			std::stringstream ss;
			ss << difftime(now, start) << "\tFROM "<< dlp.node_id<<"\tCLNT SEG REP\t\t"  << "/csu/cs557/" << dlp.filename << " ";
			for(unsigned int i = 0; i < dlp.seg_numbers.size(); ++i) {
				ss << dlp.seg_numbers[i] << " ";
				if(writeFileSegment(dlp.filename, dlp.seg_numbers[i], dlp.seg_data[i].data())) {
					completed_files[dlp.filename] = true;
					ss << "\n";
					ss << difftime(now, start) << "\t COMPLETE\t" <<dlp.filename << "\n";
					ss.flush();
					log(ss.str());
					
					return;
				}
			}
			ss << "\n";
			ss.flush();
			log(ss.str());
			//if(++dlcount[dlp.filename] > 7) {
					dlcount[dlp.filename] = 0;
					NtTPacket ntt;
					int loc = find(node_pkt.task_files.begin(), node_pkt.task_files.end(), dlp.filename) - node_pkt.task_files.begin();
					std::vector<int> tmp; tmp.push_back(loc);
					createNtTPacket(&ntt, tmp);
					char buf[ntt.size()];
					ntt.hton(buf);
					int status, sockfd;
					struct addrinfo hints, *servinfo, *i;
					memset(&hints, 0, sizeof hints);
					hints.ai_family = AF_INET; // IPV4
					hints.ai_socktype = SOCK_DGRAM;
					hints.ai_flags = AI_PASSIVE; // should be the same as the manager's IP
					char port[5];
					sprintf(port, "%d", node_pkt.tport);
					if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
						std::cerr << "connection: getaddrinfo() failed" << std::endl;
						exit(1);
					}
					/* loop through results and find one we can connect to */
					for (i = servinfo; i != NULL; i = i->ai_next) {
						if ((sockfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol))
								== -1) {
							perror("connection socket");
							continue;
						}
						break;
					}
					int numbytes;
					usleep(node_pkt.packet_delay*1000);
					if ((numbytes = sendto(sockfd, buf, ntt.size(), 0, i->ai_addr, i->ai_addrlen)) == -1) {
						perror("talker: sendto");
						exit(1);
					}
					time(&now);
					ss.str(std::string());
					ss << ntt.node_id <<".out";
					ss.flush();
					std::ofstream log(ss.str(), std::ios::app);
					log << difftime(now, start) << "\tTO   T\tGROUP SHOW INTEREST\t";
					log << node_pkt.task_files[loc] << "\n";
					log.flush();
					log.close();
					freeaddrinfo(servinfo);
					close(sockfd);
		//		}
		break;		
		}
		default: {
			std::cerr << "Node recieved packet of unknown type. ["<< msgtype <<"]" << std::endl;
			exit(1);
		}
	}
}

bool exists(std::string filename)
{
  std::ifstream ifile(filename);
  return ifile;
}

bool Node::writeFileSegment(std::string filename, int segment, char *data) {
	int size = getFileSize(filename);
	std::stringstream ss;
	ss << node_pkt.node_id <<"-_csu_cs556_" << filename;
	std::string filename1 = ss.str();
	if(!exists(filename1)) {
		std::ofstream os(filename1.c_str());
		char buf[size];
		os.write(buf,size);
		os.close();
	}
	int pos = segment * 32;
	std::fstream s(filename1);
	s.seekp(pos, std::ios_base::beg);
	int len = 32;
	if((pos+len) >= size) {
		len = size - pos;
	}
	s.write(data, len);
	timeout_count[filename] = 0;
	node_files[filename][segment/8] |= 1 << (segment%8);
	bool flag = true;
	int size1 = getFileSize(filename);
	int len1 = size1 % 32 == 0 ? size1 / 32 : (size1/32)+1;
	for(int i = 0; i < len1; ++i) {
		if((node_files[filename][i/8] & (1 << (i%8) )) == 0 ) {
			flag = false;
		}
	}
	s.close();
	return flag;
}

void Node::createDLPacket(DownloadPacket *dlp, std::string filename, std::vector<int> seg_numbers, std::vector<std::array<char, 32>> seg_data) {
	dlp->node_id = node_pkt.node_id;
	dlp->filename = filename;
	for(auto i : seg_numbers) {
		dlp->seg_numbers.push_back(i);
	}
	dlp->seg_data = std::vector<std::array<char,32>>(seg_data.size());		
	for(unsigned int i = 0; i < seg_data.size(); ++i) {
		memcpy(dlp->seg_data[i].data(), seg_data[i].data(), 32);
	}
}

void Node::createDLRPacket(DownloadRequestPacket *dlr, std::vector<uint16_t> seg, std::string filename) {
	dlr->msgtype = 5;
	dlr->node_id = node_pkt.node_id;
	dlr->port = atoi(myTCPport);
	dlr->filename = filename;
	dlr->num_seg = seg.size();
	dlr->segments = seg;
}

std::vector<std::pair<int,NodeFileInfo>> Node::choose8(std::string filename) {

	int seg_count = 0;
	int round_count = 0;
	std::vector<std::pair<int,NodeFileInfo>> nodes;
	std::set<int> segments;
	while(seg_count < 8) {
		for (auto info : group_assignments[filename]) {
			if(info.node_id !=  node_pkt.node_id) {
				int size = getFileSize(filename);
				int len = size / 32;
				int len1 = size % 32 == 0 ? len : len+1;
				len = len1 % 8 == 0 ? len1/8 : (len1/8)+1;
				char result[len];
				memset(result, 0, len);
				for(int j = 0; j < len1; ++j) {
					bool bit1 = !(((node_files[filename][j/8]) & (1 << (j%8) )) != 0 );
					bool bit2 = ((info.downloaded_segments[j/8] & (1 << (j%8) )) != 0 );
					bool bit = bit1 & bit2;
					result[j/8] |= bit << (j%8); 
				}
/*
				std::cout << node_pkt.node_id <<" has ";
				for(int i = 0; i < len1; ++i) {
					std::cout <<  ((node_files[filename][i/8] & (1 << (i%8) )) != 0 );
				} std::cout << std::endl;
				std::cout << info.node_id <<" has ";
				for(int i = 0; i < len1; ++i) {
					std::cout <<  ((info.downloaded_segments[i/8] & (1 << (i%8) )) != 0 );
				} std::cout << std::endl;
*/
				int index = rand() % len1;
					for(int i = 0; i < len1; ++i) {
					int k = (index+i) % (len*8);
					if( (result[(k)/8] & (1 << (k%8) )) != 0 ) {
						if((segments.insert(k).second == true) && (seg_count < 8)) {
							seg_count++;
							nodes.push_back(std::pair<int,NodeFileInfo>(k, info));	
							break;
						}
					} 						
				}
			}
		}
		if(round_count++ > 8){
			break;
		}
	}
	return nodes;
}

void Node::printGroupAssignments() {
	std::cout << "--- Group Assignments for Node "<< node_pkt.node_id<<" ---" << std::endl;
	for (auto it : group_assignments) {
   	std::cout << it.first << " => " << "{";
		for(unsigned int i = 0; i < it.second.size()-1; ++i) {
			std::cout << it.second[i].node_id << ", ";
		}
		std::cout << it.second[it.second.size()-1].node_id <<"}" << std::endl;
	}
	std::cout << "---------------------------------" << std::endl;
}

void Node::log(std::string str) {
	std::ofstream log(name.c_str(), std::ios::app);
	log <<  str;
	log.close();
}

bool Node::hasFile(std::string filename) {
	bool result = std::find(node_pkt.filenames.begin(), node_pkt.filenames.end(),
						filename) != node_pkt.filenames.end();
	return result;
}

long Node::getFileSize(std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

int main(int argc, char* argv[]){
	int sockfd = Connection::connectToPort(argv[0], true);
	const char* message = "node reporting";
	send(sockfd, message, 14, 0);
	long pkt_size;
	recv(sockfd, &pkt_size, sizeof(long), 0);
	pkt_size = ntohl(pkt_size);
	char buf[pkt_size];
	recv(sockfd, &buf, pkt_size, 0);
	NodeConfigPacket pkt(buf);
	Node node(pkt); 
}

void Node::createIHavePacket(IHavePacket * pkt, std::string filename, int v) {
	pkt->msgtype = v;
	pkt->node_id = node_pkt.node_id;
	pkt->port = atoi(myTCPport);
	pkt->filename = filename;
	int size = getFileSize(filename);
	int segs = size / 32;
	segs = size % 32 == 0 ? segs : segs+1;
	pkt->total_segments = segs;
	pkt->downloaded_segments = (char*)malloc(segs*sizeof(char));
	memcpy(pkt->downloaded_segments, node_files[filename], segs);
}

void Node::createNtTPacket(NtTPacket *ntt, std::vector<int> group) {
	ntt->msgtype = 1;
	ntt->node_id = node_pkt.node_id;
	ntt->node_port = atoi(myTCPport);
	ntt->num_files = group.size();
	for(auto i : group) {
		ntt->filenames.push_back(node_pkt.task_files[i]);
		if(hasFile(node_pkt.task_files[i])) {
			if(node_pkt.shares[i] == 1) {
				ntt->sharetypes.push_back(3);
			} else {
				ntt->sharetypes.push_back(0); // Have don't won't share
			}
		} else {
			if(node_pkt.shares[i] == 1) {
				ntt->sharetypes.push_back(2);
			} else {
				ntt->sharetypes.push_back(1);			
			}
		}
	}
}

void Node::listen() {
   struct fd_state *state[FD_SETSIZE];
   //struct sockaddr_in sin;
   int i, maxfd;
   fd_set readset, writeset, exset;
	for (i = 0; i < FD_SETSIZE; ++i) {
        state[i] = NULL;
	}
	makeNonblocking(myTCPsocket);
	/* listen for incoming connect()'s (this does not block); */
	if (::listen(myTCPsocket, 25) == -1) {
		perror("node listen");
		exit(1);
	}
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&exset);
	startFileTimers();
	struct timeval tmv;
//	setsockopt(myTCPsocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	while(true) { 
		timersManager_->NextTimerTime(&tmv);
		if (tmv.tv_sec == 0 && tmv.tv_usec == 0) {
		        // The timer at the head on the queue has expired 
		        timersManager_->ExecuteNextTimer();
			continue;
		}
		if (tmv.tv_sec == MAXVALUE && tmv.tv_usec == 0){
		        // There are no timers in the event queue 
			tmv.tv_sec = 15; // ten seconds timeout
			exitflag = true;
		}
		maxfd = myTCPsocket;
		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		FD_ZERO(&exset);
		FD_SET(myTCPsocket, &readset);
        for (i=0; i < FD_SETSIZE; ++i) {
            if (state[i]) {
                if (i > maxfd)
                    maxfd = i;
                FD_SET(i, &readset);
                if (state[i]->writing) {
                    FD_SET(i, &writeset);
                }
            }
        }
		
		int retval = select(maxfd+1, &readset, &writeset, &exset, &tmv);
        if (retval <= 0) {
			if(retval == 0) {
				if(exitflag) {
					std::cout << "Node "<< node_pkt.node_id <<" terminating." << std::endl;
					return;
				} else {
					// Timer expired, Hence process it 
				    timersManager_->ExecuteNextTimer();
					// Execute all timers that have expired.
					timersManager_->NextTimerTime(&tmv);
					while(tmv.tv_sec == 0 && tmv.tv_usec == 0){
					   // Timer at the head of the queue has expired 
					    timersManager_->ExecuteNextTimer();
						timersManager_->NextTimerTime(&tmv);
					}
				}
			} else {
				perror("select");
	            return;
			}
        } else {
		    if (FD_ISSET(myTCPsocket, &readset)) {
		        struct sockaddr_storage ss;
		        socklen_t slen = sizeof(ss);
		        int fd = accept(myTCPsocket, (struct sockaddr*)&ss, &slen);
		        if (fd < 0) {
		            perror("accept");
		        } else if (fd > FD_SETSIZE) {
		            close(fd);
		        } else {
		            makeNonblocking(fd);
		            state[fd] = alloc_fd_state();
		        }
		    }
		    for (i=0; i < maxfd+1; ++i) {
		        if (i == myTCPsocket)
		            continue;
		        if (FD_ISSET(i, &readset)) {
						int num_bytes;
						char buf[1000];
						num_bytes = recv(i, buf, 1000, 0);
						if (num_bytes == -1) {
							perror("recv");
							exit(1);
						}
						handlePacket(buf);
		           free(state[i]);
		           state[i] = NULL;
						close(i);
		        }
		    }
		}
	}
}

long Node::ProcessTimer(int p)
{
	struct timeval tv;
	getTime(&tv);
	return tv.tv_sec;
}

void Node::addTimer(TimerCallback *tcb, unsigned short starttime) {
	starttime *= 1000; // convert to miliseconds
	// Create callback classes and set up pointers
	// Add timers to the event queue and specify the timer in ms.
	exitflag = false;
	timersManager_->AddTimer(starttime, tcb);
}




