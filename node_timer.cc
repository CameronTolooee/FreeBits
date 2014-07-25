/*
 * node_timer.cc
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 *
 * Implementation of the provided timer library that will talk to the tracker at
 * the specified interval.
 *
 */
#include "node_timer.h"
#include "connection.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <netdb.h>

/*
* The return value from expire is used 
* to indicate what should be done with the timer
*  = 0   Add timer again to queue
*  > 0   Set new_timeout as timeout value for timer 
*  < 0   Discard timer 
*/  

int NodeTimer::Expire()
{
	char port[5];
	sprintf(port, "%d", tport);
	char buf[ntt.size()];
	ntt.hton(buf);
	int status, sockfd;
	struct addrinfo hints, *servinfo, *i;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPV4
	hints.ai_socktype = SOCK_DGRAM;
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
		break;
	}
	int numbytes;
	if ((numbytes = sendto(sockfd, buf, ntt.size(), 0, i->ai_addr, i->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
	time_t now;
	time(&now);
	std::stringstream ss;
	ss << ntt.node_id <<".out";
	ss.flush();
	std::ofstream log(ss.str(), std::ios::app);
	log << difftime(now, start) << "\tTO   T\tGROUP SHOW INTEREST\t";
	for(auto i : ntt.filenames)
		log << i << " ";
	log << "\n";
	log.flush();
	log.close();

	freeaddrinfo(servinfo);
	close(sockfd);
	return -1;
}

void TestApp::start()
{
	struct timeval tmv;
	int status;

	// Change while condition to reflect what is required for Project 1
	// ex: Routing table stabalization. 
	while (1) {
		timersManager_->NextTimerTime(&tmv);
		if (tmv.tv_sec == 0 && tmv.tv_usec == 0) {
		        // The timer at the head on the queue has expired 
		        timersManager_->ExecuteNextTimer();
			continue;
		}
		if (tmv.tv_sec == MAXVALUE && tmv.tv_usec == 0) {
		        // There are no timers in the event queue 
		        break;
		}
		  
		/* The select call here will wait for tv seconds before expiring 
		 * You need to  modifiy it to listen to multiple sockets and add code for 
		 * packet processing. Refer to the select man pages or "Unix Network 
		 * Programming" by R. Stevens Pg 156.
		 */
		status = select(0, NULL, NULL, NULL, &tmv);
		if (status < 0){
			// This should not happen
			fprintf(stderr, "Select returned %d\n", status);
		}else{
			if (status == 0){
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
			if (status > 0){
				// The socket has received data.
				// Perform packet processing.
		    
			}
		}
	}
	return;
}

long TestApp::ProcessTimer(int p)
{
	struct timeval tv;
	getTime(&tv);
	return tv.tv_sec;
}

void TestApp::addTimer(TimerCallback *tcb, unsigned short starttime) {
	starttime *= 1000; // convert to miliseconds
	// Create callback classes and set up pointers
	// Add timers to the event queue and specify the timer in ms.
	timersManager_->AddTimer(starttime, tcb);
}

TestApp::TestApp() {
	// Create the timer event management class
	timersManager_ = new Timers();
}


