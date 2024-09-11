#pragma once
void recvAndParsePayload(int fd);
void readCallback(evutil_socket_t fd, short events, void* arg);
