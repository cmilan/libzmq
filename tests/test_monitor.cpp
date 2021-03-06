/*
    Copyright (c) 2007-2012 iMatix Corporation
    Copyright (c) 2011 250bpm s.r.o.
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../include/zmq.h"
#include "../include/zmq_utils.h"
#include <string.h>
#include "testutil.hpp"

static int events;

typedef void *ZmqSocket;
ZmqSocket rep, req;

void socket_monitor (ZmqSocket s, int event_, zmq_event_data_t *data_)
{
    assert(s == rep || s == req);

    const char *addr = "tcp://127.0.0.1:5560";
    // Only some of the exceptional events could fire
    switch (event_) {
    // listener specific
    case ZMQ_EVENT_LISTENING:
        assert (s == rep);
        assert (data_->listening.fd > 0);
        assert (!strcmp (data_->listening.addr, addr));
        events |= ZMQ_EVENT_LISTENING;
        break;
    case ZMQ_EVENT_ACCEPTED:
        assert (s == rep);
        assert (data_->accepted.fd > 0);
        assert (!strcmp (data_->accepted.addr, addr));
        events |= ZMQ_EVENT_ACCEPTED;
        break;
    // connecter specific
    case ZMQ_EVENT_CONNECTED:
        assert (s == req);
        assert (data_->connected.fd > 0);
        assert (!strcmp (data_->connected.addr, addr));
        events |= ZMQ_EVENT_CONNECTED;
        break;
    case ZMQ_EVENT_CONNECT_DELAYED:
        assert (s == req);
        assert (data_->connect_delayed.err != 0);
        assert (!strcmp (data_->connect_delayed.addr, addr));
        events |= ZMQ_EVENT_CONNECT_DELAYED;
        break;
    // generic - either end of the socket
    case ZMQ_EVENT_CLOSE_FAILED:
        assert (data_->close_failed.err != 0);
        assert (!strcmp (data_->close_failed.addr, addr));
        events |= ZMQ_EVENT_CLOSE_FAILED;
        break;
    case ZMQ_EVENT_CLOSED:
        assert (data_->closed.fd != 0);
        assert (!strcmp (data_->closed.addr, addr));
        events |= ZMQ_EVENT_CLOSED;
        break;
    case ZMQ_EVENT_DISCONNECTED:
        assert (data_->disconnected.fd != 0);
        assert (!strcmp (data_->disconnected.addr, addr));
        events |= ZMQ_EVENT_DISCONNECTED;
        break;
    default:
        // out of band / unexpected event
        assert (0);
    }
}

int main (void)
{
    int rc;

    //  Create the infrastructure
    void *ctx = zmq_init (1);
    assert (ctx);
    // set socket monitor
    rc = zmq_ctx_set_monitor (ctx, socket_monitor);
    assert (rc == 0);
    rep = zmq_socket (ctx, ZMQ_REP);
    assert (rep);

    rc = zmq_bind (rep, "tcp://127.0.0.1:5560");
    assert (rc == 0);

    req = zmq_socket (ctx, ZMQ_REQ);
    assert (req);

    rc = zmq_connect (req, "tcp://127.0.0.1:5560");
    assert (rc == 0);

    bounce (rep, req);

    // Allow a window for socket events as connect can be async
    zmq_sleep (1);

    //  Deallocate the infrastructure.
    rc = zmq_close (req);
    assert (rc == 0);

    // Allow for closed or disconnected events to bubble up
    zmq_sleep (1);

    rc = zmq_close (rep);
    assert (rc == 0);

    zmq_sleep (1);

    zmq_term (ctx);

    // We expect to at least observe these events
    assert (events & ZMQ_EVENT_LISTENING);
    assert (events & ZMQ_EVENT_ACCEPTED);
    assert (events & ZMQ_EVENT_CONNECTED);
    assert (events & ZMQ_EVENT_CLOSED);
    assert (events & ZMQ_EVENT_DISCONNECTED);

    return 0 ;
}

