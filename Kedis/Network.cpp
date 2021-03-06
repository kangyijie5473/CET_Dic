//
// Created by kang on 17-10-1.
//

#include "Network.h"
#include "protocol.h"
#include "TheadPool.h"
int Network::initDb() {
    static struct timeval _timeout = {1, 500000};
    _context = redisConnectWithTimeout(DB_HOST, DB_PORT, _timeout);
    if(_context == NULL || _context->err){
        if(_context){
            printf("Redis Connection error : %s\n",_context->errstr);
            redisFree(_context);
        }else{
            printf("Redis Connection error : can't allocate redis context\n");
        }
        return -1;
    }
}
int Network::Listen() {
    int optrval = 1;
    sock_len = sizeof(struct sockaddr_in);

    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&optrval,sizeof(int));

    memset(&_server_addr, 0, sizeof(struct sockaddr_in));
    _server_addr.sin_family = AF_INET;
    _server_addr.sin_port = htons(_port);
    _server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    this->setNonBlocking(_socket_fd);

    bind(_socket_fd, (struct sockaddr *)&_server_addr, sizeof(struct sockaddr_in));
    listen(_socket_fd, LISTEN_LEN);
    return 1;
}
bool Network::setNonBlocking(int fd) {
    if(fcntl(fd, F_SETFL, fcntl(_socket_fd, F_GETFD, 0)|O_NONBLOCK) == -1){
        return false;
    }else
        return true;
}
int Network::recvMessage(int fd, char *buffer, std::string &job_type) {
    struct MessageHeader myhead;
    if(read(fd, &myhead, HEADER_SIZE) < HEADER_SIZE)
        return -1;

    job_type = std::string(doJobProc[myhead.typeCode]);
    std::cout << job_type << ":id is " << myhead.typeCode << std::endl;
    if(read(fd,buffer, myhead.messageSize) < myhead.messageSize)
        return -1;
    else
        return 0;
}
int Network::sendMessage(int fd, char *buffer) {
    int len  = strlen(buffer);
    if(write(fd, buffer, len) < len)
        return -1;
    else
        return 0;
}
int Network::startMainLoop() {
    _kdpfd = epoll_create(MAX_POLL_SIZE);
    std::cout << "start" << std::endl;
    _ev.events = EPOLLIN | EPOLLET;
    _ev.data.fd = _socket_fd;
    if(epoll_ctl(_kdpfd, EPOLL_CTL_ADD, _socket_fd, &_ev) < 0){
        perror("epoll_ctl");
        return -1;
    }
    int curfds = 1;
    int acceptCount = 0;
    while(1){
        _nfds = epoll_wait(_kdpfd, _events, curfds, -1);
        if(_nfds == -1){
            perror("epoll_wait");
            continue;
        }
        std::cout << "number is " << _nfds << std::endl;
        for(int n = 0; n < _nfds; n++){
            if(_events[n].data.fd == _socket_fd){
                _conn_fd = accept(_socket_fd, (struct sockaddr *)&_client_addr, &sock_len);
                if(_conn_fd < 0){
                    perror("accept error");
                    continue;
                }
                //std::cout << "accept fd is " + _conn_fd << std::endl;
                acceptCount++;

                if(curfds >= MAX_POLL_SIZE){
                    close(_conn_fd);
                    continue;
                }
                if(this->setNonBlocking(_conn_fd) < 0){
                    perror("set_non_blocking error");
                }

                _ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                _ev.data.fd = _conn_fd;

                if(epoll_ctl(_kdpfd, EPOLL_CTL_ADD, _conn_fd, &_ev) < 0){
                    return -1;
                }

                curfds++;
                continue;
            }else if(_events[n].events & EPOLLRDHUP){
                std::cout << "HUP" << std::endl;
                struct epoll_event ee = {0};
                ee.events |= EPOLLRDHUP;
                ee.events |= EPOLLIN;
                ee.data.fd = _events[n].data.fd;
                epoll_ctl(_socket_fd, EPOLL_CTL_DEL, _events[n].data.fd, &ee);
                continue;
            }else if(_events[n].events & EPOLLIN){
                std::cout << "add task" << std::endl;
                ThreadPool::getInstance().addTask(_events[n].data.fd);
            }

        }
    }
}
