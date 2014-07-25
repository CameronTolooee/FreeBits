/*
 * manager.h
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 * 
 *  Header file for the manager execuatable.
 * 
 */

#ifndef MANAGER_H_
#define MANAGER_H_
#include "conf_file_parser.h"
#include "wireformats.h"

class Manager {
public:
	Manager(std::string file);
	virtual ~Manager();
	void forkProcesses(char* server_port, ConfFileInfo file_info);
	void createConfigurations(ConfFileInfo info, std::vector<NodeConfigPacket*> &configs, uint16_t tport);
private:
	std::string file;
};

#endif /* MANAGER_H_ */
