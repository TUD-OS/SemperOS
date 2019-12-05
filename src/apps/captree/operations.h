
#pragma once

#include <m3/session/Session.h>
#include <m3/com/SendGate.h>
#include <m3/com/RecvGate.h>
#include <m3/com/GateStream.h>

class CapServerSession : public m3::Session {
public:
    CapServerSession(const m3::String &name) : m3::Session(name),
            _sgate(m3::SendGate::bind(obtain(1).start())) {
    }

enum CapOperation {
    SIGNAL,
    COUNT
};

m3::Errors::Code signal() {
    m3::GateIStream reply = m3::send_receive_vmsg(_sgate, CapOperation::SIGNAL);
    reply >> m3::Errors::last;
    return m3::Errors::last;
}

private:
    m3::SendGate _sgate;

};
