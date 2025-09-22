
#include "../../include/io/CreateSocket.hpp"



#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int setNonBlocking(int fd) {
    int flags = fcntl(fd,F_GETFL,0);
    if (flags < 0)
        return -1;
    return fcntl(fd, F_SETFL,flags | O_NONBLOCK);
}


int createSocket(const std::string& ip , int port) {
    int fd = ::socket(AF_INET,SOCK_STREAM,0);
    if (fd < 0) {
        std::perror("Error: Socket function");
        return -1;
    }
    int on = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        std::perror("Error: setsockopt SO_REUSEADRR");
        ::close(fd);
        return -1;
    }
    sockaddr_in addr;
    std::memset(&addr, 0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (ip.empty() || ip == "0.0.0.0"){
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
            std::perror("Error : inet_pton");
            close(fd);
            return -1;
        }
    }
    if (::bind(fd,reinterpret_cast<sockaddr *>(&addr),sizeof(addr)) < 0) {
        std::perror("Error : bind");
        close(fd);
        return -1;
    }
    if (::listen(fd,128) < 0) {
        std::perror("Error : listen");
        ::close(fd);
        return -1;
    }
    if (::setNonBlocking(fd) < 0) {
        std::perror("Error : setNonBlock");
        close(fd);
        return -1;
    }   
    std::cout << "the server create socket and listening" << std::endl;
    return fd;   
}