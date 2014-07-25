/*
 * node.h
 *
 *  Created on: Mar 19, 2014
 *      Author: ctolooee
 * 
 * Header file for the node timer.
 * 
 */

#ifndef NODE_TIMER_H_ 
#define NODE_TIMER_H_
#include "timers.hh"
#include "wireformats.h"
class TestApp {
public:
	TestApp();
	void start();
  	long ProcessTimer(int p);
	void addTimer(TimerCallback *tbc, unsigned short starttime);

protected:
	Timers *timersManager_;
};


class NodeTimer : public TimerCallback {
public:
    NodeTimer(unsigned short node_id, NtTPacket ntt, int tport, time_t start) 
		: node_id(node_id), ntt(ntt), tport(tport), start(start) { }
    ~NodeTimer() { }
	TestApp *app_;
	int Expire();
	unsigned short node_id;
	NtTPacket ntt;
	int tport;
	time_t start;
};
#endif
