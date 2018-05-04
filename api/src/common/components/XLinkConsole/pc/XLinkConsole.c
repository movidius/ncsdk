/*
* Copyright 2017 Intel Corporation.
* The source code, information and material ("Material") contained herein is
* owned by Intel Corporation or its suppliers or licensors, and title to such
* Material remains with Intel Corporation or its suppliers or licensors.
* The Material contains proprietary information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright laws and treaty
* provisions.
* No part of the Material may be used, copied, reproduced, modified, published,
* uploaded, posted, transmitted, distributed or disclosed in any way without
* Intel's prior express written permission. No license under any patent,
* copyright or other intellectual property rights in the Material is granted to
* or conferred upon you, either expressly, by implication, inducement, estoppel
* or otherwise.
* Any license under such intellectual property rights must be express and
* approved by Intel in writing.
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
#define MVLOG_UNIT_NAME xLinkConsole
#include "mvLog.h"


void* shellThreadWriter(void* ctx){

    int* context = ctx;
    streamId_t cId = (streamId_t)context[0];
    int connfd = (streamId_t)context[1];
    while(1){

        char str[100];
        int bytes = read(connfd,str, 100);
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
