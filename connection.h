/*
 * connetion.h
 *
 *  Created on: Mar 20, 2014
 *      Author: ctolooee
 *
 *      Because programming c sockets really sucks, we're gonna wrap
 *      a class around the parts we need so I only have to write it
 *      once....
 */

#ifndef CONNETION_H_
#define CONNETION_H_
#include <ostream>
#include <iomanip>
struct HexCharStruct
{
  unsigned char c;
  HexCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs)
{
  return (o << std::hex << std::setw(2) << std::setfill('0') << (int)hs.c);
}

inline HexCharStruct hex(unsigned char _c)
{
  return HexCharStruct(_c);
}

class Connection {
public:
	Connection();
	virtual ~Connection();
	static int connectToPort(unsigned short port, bool isTCP);
	static int connectToPort(const char* port, bool isTCP); /* assumes local ip addr for this proj */
	static int bindToPort(const char* port, bool isTCP); /* usually port = "0" for dynamic assignment */
	static void getPort(int sockfd, char * port);
	static void getAddr(int sockfd, struct sockaddr_in * sin);
	static int acceptConnection(int sockfd);
	//static void send(int sockfd, Serializable *data);

};

#endif /* CONNETION_H_ */
