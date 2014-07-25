/*
 * conf_file_parser.h
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 *
 * Provides structs and class declarations for parser for the manager.conf file.
 * 
 */

#ifndef CONF_FILE_PARSER_H_
#define CONF_FILE_PARSER_H_
#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <netinet/in.h>

struct PacketInfo {
	const short node_id;
	const int delay;
	const float packet_drop_prob;
};

struct FileLocation {
	const short node_id;
	const std::string filename;
};

struct DownloadTask {
	const short node_id;
	const std::string filename;
	const int start_time;
	const int share;
};

struct ConfFileInfo {
	int num_nodes;
	int timeout; /* in seconds */
	std::vector<PacketInfo> packet_info;
	std::vector<FileLocation> file_locations;
	std::vector<DownloadTask> download_tasks;
};

class ConfFileParser {
public:
	ConfFileParser(std::string filename);
	virtual ~ConfFileParser();
	ConfFileInfo readFile();

private:
	void readPacketInfo(ConfFileInfo &info, std::ifstream &handle, std::string line);
	void readTasks(ConfFileInfo &info, std::ifstream &handle, std::string line);
	void readLocations(ConfFileInfo &info, std::ifstream &handle, std::string line);
	bool isCommentOrBlank(std::string line);
	std::string filename;
};

std::ostream& operator<<(std::ostream& os, const ConfFileParser);


#endif /* CONF_FILE_PARSER_H_ */
