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

#pragma once

#include <base/Common.h>
#include <base/util/String.h>

#include <m3/stream/Standard.h>
#include <m3/pipe/AggregateDirectPipe.h>

namespace m3 {

class LoadGenWorkLoop;

class LoadGenWorkItem {
    enum WorkSteps {
        ConnectRequest,
        RecvHeader,
        RecvPayload
    };

public:
    LoadGenWorkItem(AggregateDirectPipe *pipe, String &server, size_t maxRounds,
            size_t rxbytesTarget1, size_t rxbytesTarget2, LoadGenWorkLoop *wl)
        : _step(ConnectRequest), _round(0), _maxRounds(maxRounds), _rxbytes(0),
          _rxbytesTarget1(rxbytesTarget1), _rxbytesTarget2(rxbytesTarget2),
          _pipe(pipe), _label(pipe->sgate()->label()), _server(server), _wl(wl) {
    }

    void work(DTU::Message *msg = nullptr);

    void shutdown() {
        // send abort signal
        int input = 0;
        _pipe->writer()->send((void*)&input, sizeof(int));
//        cout << "sent shutdown\n";
    }

    label_t label() {
        return _label;
    }

private:
    WorkSteps _step;
    int _round;
    int _maxRounds;
    size_t _rxbytes;
    size_t _rxbytesTarget1;
    size_t _rxbytesTarget2;
    AggregateDirectPipe *_pipe;
    label_t _label;
    String _server;
    LoadGenWorkLoop *_wl;
};

struct LoadGenListItem : public SListItem {
    LoadGenListItem(LoadGenWorkItem *item) : it(item) {}
    LoadGenWorkItem *it;
};

class LoadGenWorkLoop {
    static const size_t MAX_ITEMS   = 16;
public:
    LoadGenWorkLoop() : _permanents(), _count(), _countRgates() {
    }

    bool has_items() const {
        return _count > _permanents;
    }

    void stop() {
        _permanents = _count;
    }

    void add_rgate(RecvGate *rgate) {
        assert(_countRgates < MAX_ITEMS);
        _rgates[_countRgates++] = rgate;
    }

    void add(LoadGenWorkItem *item, bool permanent) {
        assert(_count < MAX_ITEMS);
        _items[_count++] = item;
        if(permanent)
            _permanents++;
    }

    void remove(LoadGenWorkItem *item) {
        for(size_t i = 0; i < MAX_ITEMS; ++i) {
            if(_items[i] == item) {
                _items[i] = nullptr;
                for(++i; i < MAX_ITEMS; ++i)
                    _items[i - 1] = _items[i];
                _count--;
                break;
            }
        }
    }
    void run() {
        // send initial requests
        for(size_t i = 0; i < _count; ++i) {
            if(_items[i])
                _items[i]->work();
        }
        size_t epid[MAX_ITEMS];
        for(size_t i = 0; i < _countRgates; i++)
            epid[i] = _rgates[i]->epid();

        size_t selectedEP = 0;
        while(_count > _permanents) {

            // now we should wait on the Gate and check the msg's label
            // then search for the label in the recvlist
            DTU::Message *msg = nullptr;

            while(!msg) {
                DTU::get().wait();
                for(selectedEP = selectedEP % _countRgates; selectedEP < _countRgates; selectedEP++) {
                    msg = DTU::get().fetch_msg(epid[selectedEP]);
                    if(msg) {
                        // be fair and let the other EPs act as well
                        selectedEP++;
                        break;
                    }
                }
            }

            for(size_t i = 0; i < _count; ++i) {
                if(_items[i] && _items[i]->label() == msg->label) {
//                    cout << "found work for " << _items[i]->label() << "\n";
                    _items[i]->work(msg);
                    break;
                }
            }
        }
    }

private:
    uint _permanents;
    size_t _count;
    size_t _countRgates;
    RecvGate *_rgates[MAX_ITEMS];
    LoadGenWorkItem *_items[MAX_ITEMS];
};

}
