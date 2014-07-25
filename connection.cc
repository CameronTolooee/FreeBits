/*
 * connetion.cc
 *
 *  Created on: Mar 20, 2014
 *      Author: ctolooee
 *
 * Desigend to be a wrapper class around some of the c-socket functionality that
 * can get pretty long-winded.
 *
 */

#include "connection.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <iostream>

Connection::Connection() {

}

Connection::~Connection() {
}

int Connection::connectToPort(unsigned short port, bool isTCP) {
	char c_port[5];
	sprintf(c_port, "%d", port);
	return connectToPort(c_port, isTCP);
}

int Connection::connectToPort(const char* port, bool isTCP) { /* assumes local ip addr for this proj */
	int status, sockfd;
	struct addrinfo hints, *servinfo, *i;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPV4
	hints.ai_socktype = isTCP ? SOCK_STREAM : SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // should be the same as the manager's IP
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
		if(isTCP) {		
			if ((connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) != 0) {
				close(sockfd);
				perror(port);
				return -1;
			}
		}
		break;
	}
	freeaddrinfo(servinfo);
	return sockfd;
}

int Connection::bindToPort(const char* port, bool isTCP) { /* usually port = "0" for dynamic assignment */
	int status, sockfd, yes = 1;
	struct addrinfo hints, *servinfo, *i;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPV4
	hints.ai_socktype = isTCP ? SOCK_STREAM : SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me

	/* Prepare addr structs */
	if ((status = getaddrinfo(NULL, "0", &hints, &servinfo)) != 0) {
		std::cerr << "getaddrinfo() failed" << std::endl;
		exit(1);
	}

	/* loop through results and find one we can connect to */
	for (i = servinfo; i != NULL; i = i->ai_next) {
		if ((sockfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol))
				== -1) {
			perror("socket");
			continue;
		}
		if(isTCP) {
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
					== -1) {
				perror("setsockopt");
				exit(1);
			}
		}			

		if ((bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}
		break;
	}
	if (i == NULL) {
		std::cerr << "failed to connect to socket" << std::endl;
		exit(1);
	}
	freeaddrinfo(servinfo);
	return sockfd;
}

void Connection::getPort(int sockfd, char* port) {
	struct sockaddr_in sin;
	unsigned int len = sizeof(sin);
	if (getsockname(sockfd, (struct sockaddr *) &sin, &len) == -1) {
		perror("getsockname");
	}
	sprintf(port, "%d", ntohs(sin.sin_port));
}

void Connection::getAddr(int sockfd, struct sockaddr_in *sin) {
	unsigned int len = sizeof(*sin);
	if (getsockname(sockfd, (struct sockaddr *) sin, &len) == -1) {
		perror("getsockname");
	}
}

int Connection::acceptConnection(int sockfd) {
	struct sockaddr_storage addr;
	unsigned int addrlen = sizeof addr;
	return accept(sockfd, (struct sockaddr *) &addr, &addrlen);
}
