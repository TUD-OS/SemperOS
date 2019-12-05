/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of SemperOS.
 *
 * SemperOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * SemperOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#define MSG_COUNT   1000000
#define MSG_SIZE    256

//const char *socket_path = "./socket";
const char *socket_path = "\0hidden";

static void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    bool perf = false;
    struct sockaddr_un srvaddr;
    char buf[MSG_SIZE];
    int fd, rc, res;

    if(argc > 1 && strcmp(argv[1], "--perf") == 0)
        perf = true;

    if((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
        error("socket");

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sun_family = AF_UNIX;
    strncpy(srvaddr.sun_path, socket_path, sizeof(srvaddr.sun_path) - 1);

    if(perf) {
        for(int i = 0; i < MSG_COUNT; ++i) {
            if(sendto(fd, buf, sizeof(buf), 0, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) < 0)
                error("sendto");
        }
    }
    else {
        while((rc = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
            if((res = sendto(fd, buf, rc, 0, (struct sockaddr*)&srvaddr, sizeof(srvaddr))) != rc) {
                if(res < 0)
                    perror("sendto");
                else
                    fprintf(stderr, "Partial write\n");
            }
        }
    }
    return 0;
}
