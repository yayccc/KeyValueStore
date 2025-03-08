#pragma once

#include<arpa/inet.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>

#include<iostream>
#include<string>

#include "connect_item.h"
#include "protocol_parser.h"
#include "map_engine.h"
#include "write_ahead_log.h"


const int EPOLL_SIZE = 1024;