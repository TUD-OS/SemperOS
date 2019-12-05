
#pragma once

#include <m3/session/Session.h>
#include <m3/com/SendGate.h>
#include <m3/com/RecvGate.h>
#include <m3/com/GateStream.h>

#define NUM_SESSIONS    1
#define NUM_OBTAINS     5
#define NUM_CLIENTS     1
#define NUMCAPS         (NUM_SESSIONS * NUM_OBTAINS * NUM_CLIENTS)

class CapServerSession : public m3::Session {
public:
    CapServerSession(const m3::String &name) : m3::Session(name),
            _sgate(m3::SendGate::bind(obtain(1).start())) {
    }

enum CapOperation {
    REVCAP,
    COUNT
};

m3::Errors::Code revcap(m3::CapRngDesc caps) {
    m3::GateIStream reply = m3::send_receive_vmsg(_sgate, REVCAP, caps);
    reply >> m3::Errors::last;
    return m3::Errors::last;
}

private:
    m3::SendGate _sgate;

};
