/*
*
* Copyright (c) 2017-2018 Intel Corporation. All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <XLink.h>
#include <XLinkConsole.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#define MVLOG_UNIT_NAME xLinkConsole
#include "mvLog.h"


void* shellThreadWriter(void* ctx){

    int* context = ctx;
    streamId_t cId = (streamId_t)context[0];
    int connfd = (streamId_t)context[1];
    while(1){

        uint8_t str[100];
        int bytes = read(connfd, str, 100);
        if (bytes > 0){
            XLinkWriteData(cId, str, bytes);
        }else
            perror("read");
    }
}

void* shellThreadReader(void* ctx){

    int* context = ctx;
    streamId_t cId = (streamId_t)context[1];
    int portNum = (streamId_t)context[0];
    if(portNum < 5000){
        mvLog(MVLOG_WARN, "Port number <5000. using port number 5000");
        portNum = 5000;
    }

    pthread_t shellWriter;
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portNum);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    mvLog(MVLOG_INFO, "Device console open. Please open a telnet session on port %d to connect:", portNum);
    mvLog(MVLOG_INFO, "telnet localhost %d", portNum);
    listen(listenfd, 10);
    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
    //reuse context allocated before starting this thread
    context[0] = cId;
    context[1] = connfd;
    streamPacketDesc_t *packet;
    pthread_create(&shellWriter, NULL, shellThreadWriter, (void*) context);

    while(1){
        XLinkReadData(cId, &packet);
        write(connfd, packet->data, packet->length);
        XLinkReleaseData(cId);
    }
}
void initXlinkShell(XLinkHandler_t* handler, int portNum){
    pthread_t shellReader;
    streamId_t cId = XLinkOpenStream(handler->linkId, "console", 10*1024);
    int* context = malloc(2* sizeof(int));
    context[0] = portNum;
    context[1] = cId;
    pthread_create(&shellReader, NULL, shellThreadReader, (void*) context);
}
