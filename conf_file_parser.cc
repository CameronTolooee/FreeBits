/*
 * conf_file_parser.cc
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 *
 * Implementation of the configuration file parser.
 *
 */

#include "conf_file_parser.h"
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

ConfFileParser::ConfFileParser(std::string filename) :
		filename(filename) {
}

ConfFileParser::~ConfFileParser() {
}

ConfFileInfo ConfFileParser::readFile() {
	std::string line;
	//std::ifstream file_handle(filename.c_str());
	std::ifstream file_handle(filename);
	ConfFileInfo info = { -1, -1 };

	while (std::getline(file_handle, line)) {
		/* this line is a comment --> ignore */
		if (isCommentOrBlank(line)) {
			continue;
		} else if (info.num_nodes == -1) {
			info.num_nodes = atoi(line.c_str());
			if (info.num_nodes < 1) {
				throw "Invalid manager.conf file: number of nodes must be greater than 0";
			}
		} else if (info.timeout == -1) {
			info.timeout = atoi(line.c_str());
			if (info.timeout < 1) {
				throw "Invalid manager.conf file: timeout must be greater than 0";
			}
		} else if (info.packet_info.size() == 0) {
			readPacketInfo(info, file_handle, line);
		} else if (info.file_locations.size() == 0) {
			readLocations(info, file_handle, line);
		} else if (info.download_tasks.size() == 0) {
			readTasks(info, file_handle, line);
		}
	}
	file_handle.close();
	return info;
}

/**
 * returns true if the line should be skipped
 * else false
 */
bool ConfFileParser::isCommentOrBlank(std::string line) {
	bool result = false;

	/* find the first char that is not a space or a tab */
	size_t found = line.find_first_not_of(" \t");
	if (found != std::string::npos) {
		if (line[found] == '#') {
			result = true;
		}
	} else {
		result = true;
	}
	return result;
}

void ConfFileParser::readPacketInfo(ConfFileInfo &info, std::ifstream &handle,
		std::string line) {
	do {
		if (isCommentOrBlank(line)) {
			continue;
		}
		boost::smatch match;
		boost::regex rx("\\s*(-?\\d+)\\s+(-?\\d+)\\s+(-?\\d+).*");
		if (boost::regex_match(line, match, rx)) {
			PacketInfo pkts = { boost::lexical_cast<short>(match[1]),
					boost::lexical_cast<int>(match[2]),
					boost::lexical_cast<float>(match[3]) };
			info.packet_info.push_back(pkts);
			if (match[1] == "-1") {
				return;
			}
		} else {
			throw "Invalid manager.conf file: invalid packet delay and drop probability parameters";
		}
	} while (std::getline(handle, line));
}

void ConfFileParser::readTasks(ConfFileInfo &info, std::ifstream &handle,
		std::string line) {
	do {
		if (isCommentOrBlank(line)) {
			continue;
		}
		boost::smatch match;
		boost::regex rx("\\s*(-?\\d+)\\s+([^\\s]+)\\s+(-?\\d+)\\s+(-?\\d+).*");
		if (boost::regex_match(line, match, rx)) {
			std::string f = boost::lexical_cast<std::string>(match[2]);
			int loc = f.find_last_of("/\\");
			if(loc > 0)
				f = f.substr(loc+1);
			DownloadTask task = { boost::lexical_cast<short>(match[1]),
					f, boost::lexical_cast<int>(match[3]),
					boost::lexical_cast<int>(match[4]) };
			info.download_tasks.push_back(task);
			if (match[1] == "-1") {
				return;
			}
		} else {
			throw "Invalid manager.conf file: invalid download task parameters";
		}
	} while (std::getline(handle, line));
}

void ConfFileParser::readLocations(ConfFileInfo &info, std::ifstream &handle,
		std::string line) {

	do {
		if (isCommentOrBlank(line)) {
			continue;
		}
		boost::smatch match;
		boost::regex rx("\\s*(-?\\d+)\\s+([^\\s]+).*");
		if (boost::regex_match(line, match, rx)) {
			std::string f = boost::lexical_cast<std::string>(match[2]);
			int loc = f.find_last_of("/\\");
			if(loc > 0)
				f = f.substr(loc+1);
			FileLocation locs = { boost::lexical_cast<short>(match[1]), f};
			info.file_locations.push_back(locs);
			if (match[1] == "-1") {
				return;
			}
		} else {
			throw "Invalid manager.conf file: invalid initial file location parameters";
		}
	} while (std::getline(handle, line));
}

std::ostream& operator<<(std::ostream& os, const ConfFileInfo info) {
	os << "Number of Nodes: " << info.num_nodes << "\n";
	os << "        Timeout: " << info.timeout << "\n";
	os << "\nNodeID	Packet delay	Drop Prob\n";
	for (unsigned int i = 0; i < info.packet_info.size(); ++i) {
		os << info.packet_info[i].node_id << "\t" << info.packet_info[i].delay
				<< "\t\t" << info.packet_info[i].packet_drop_prob << "\n";
	}
	os << "\nNodeID	Filename\n";
	for (unsigned int i = 0; i < info.file_locations.size(); ++i) {
		os << info.file_locations[i].node_id << "\t"
				<< info.file_locations[i].filename << "\n";
	}
	os << "\nNodeID	Filename	starttime	share\n";
	for (unsigned int i = 0; i < info.download_tasks.size(); ++i) {
		os << info.download_tasks[i].node_id << "\t"
				<< info.download_tasks[i].filename << "\t\t"
				<< info.download_tasks[i].start_time << "\t\t"
				<< info.download_tasks[i].share << "\n";
	}
	return os;
}

#ifdef CONF_MAIN_
int main() {
	ConfFileParser parser("manager.conf");
	try {
		ConfFileInfo info = parser.readFile();
		std::cout << info << std::endl;
	} catch (char const* str) {
		std::cerr << str << std::endl;
	}
}
#endif
