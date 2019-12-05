/*
 * Copyright (C) 2019, Matthias Hille <matthias.hille@tu-dresden.de>, 
 * Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 *  This file is part of SemperOS.
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

#include "LoadGenWorkLoop.h"

using namespace m3;

static const String request("GET /index.html HTTP/1.0\r\nHost: localhost\r\nUser-Agent: ApacheBench/2.3\r\nAccept: */*\r\n\r\n");

void LoadGenWorkItem::work(DTU::Message* msg) {
    int input = 1;
    switch (_step) {
        case ConnectRequest:
            // start the connection (triggers select)
            _pipe->writer()->send((void*) &input, sizeof (int));
//            cout << "sent connection request\n";

            // send the request (triggers recvfrom)
            _pipe->writer()->send(request.c_str(), request.length());
//            cout << "sent request\n";
            _step = RecvHeader;
            _rxbytes = 0;
            break;
        case RecvHeader:
            assert(msg);
            // read the reply header (triggered by writev)
            _rxbytes += msg->length;
            _pipe->mark_read(msg);

            // move to next step if we are done
            if (_rxbytes >= _rxbytesTarget1) {
//                cout << "loadgen: finished step 2, received " << _rxbytes << "\n";
                _rxbytes = 0;
                _step = RecvPayload;
            }
            break;
        case RecvPayload:
            assert(msg);
            // wait for the reply content (triggered by sendfile)
            //                cout << "loadgen: received bytes first time: " << msg->length << "\n";
            _rxbytes += msg->length;
            _pipe->mark_read(msg);

            // move to next step if we are done
            if (_rxbytes >= _rxbytesTarget2) {
//                cout << "loadgen: finished step 3, received " << _rxbytes << ", round " << _round << " for server " << _server << "\n";
                _rxbytes = 0;
                _step = ConnectRequest;
                _round++;

                // delete the workitem if we are done, else immediately send the next request
                if (_round == _maxRounds)
                    _wl->remove(this);
                else
                    work();
            }
            break;
    }

}

