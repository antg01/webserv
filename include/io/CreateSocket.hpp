#ifndef CREATESOCKET_HPP
#define CREATESOCKET_HPP


#include <string>

int setNonBlocking(int fd);
int createSocket(const std::string &ip, int port);
#endif