/*
 * This file is part of https://github.com/LPC-language/Signal-Server
 * Copyright (C) 2024 Dworkin B.V.  All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


# include <String.h>
# include <Continuation.h>
# include <KVstore.h>
# include "~HTTP/HttpResponse.h"
# include "KVstoreExp.h"
# include "Timestamp.h"
# include "account.h"
# include "messages.h"


# define DURATION	30 * 24 * 3600

# define QUEUE		0
# define ENDPOINT	1

# define RECEIPT	5

object messages;	/* messages */
object mboxes;		/* mail boxes */

/*
 * initialize message server
 */
static void create()
{
    messages = new KVstoreExp(100, DURATION);
    mboxes = new KVstore(100);
}

/*
 * send a message
 */
atomic void send(string accountId, int deviceId, object endpoint,
		 Envelope envelope)
{
    string index, guid;
    mapping mbox;
    int notify;

    /* add to message queue */
    index = deviceId + accountId;
    mbox = mboxes[index];
    if (!mbox) {
	mboxes[index] = mbox = ([ QUEUE : ({ }) ]);
    }
    guid = envelope->guid();
    messages->add(guid, envelope);
    mbox[QUEUE] += ({ guid });

    if (endpoint && !mbox[ENDPOINT]) {
	mbox[ENDPOINT] = endpoint;
	call_out("deliver", 0, accountId, deviceId);
    }
}

static atomic void deliver(string id, int deviceId)
{
    mapping mbox;
    object endpoint;
    string guid;
    Envelope envelope;

    mbox = mboxes[deviceId + id];
    endpoint = mbox[ENDPOINT];
    if (endpoint) {
	if (sizeof(mbox[QUEUE]) != 0) {
	    guid = mbox[QUEUE][0];
	    envelope = messages[guid];
	    if (envelope) {
		endpoint->chatSendRequest("PUT", "/api/v1/message",
					  envelope->transport(), ([
		    "X-Signal-Timestamp" : new Timestamp->transport()
		]), new Continuation("delivered", id, deviceId, envelope));
	    } else {
		mbox[QUEUE] = mbox[QUEUE][1 ..];
	    }
	} else {
	    mbox[ENDPOINT] = nil;
	    endpoint->chatSendRequest("PUT", "/api/v1/queue/empty", nil, nil,
				      nil);
	}
    }
}

static atomic void delivered(string id, int deviceId, Envelope envelope,
			     int code)
{
    string guid;
    mapping mbox;

    guid = envelope->guid();
    messages[guid] = nil;
    mbox = mboxes[deviceId + id];
    mbox[QUEUE] = mbox[QUEUE][1 ..];

    if (code == HTTP_OK) {
	if (envelope->type() != RECEIPT) {
	    call_out("receipt", 0, id, deviceId, envelope);
	}

	call_out("deliver", 0, id, deviceId);
    }
}

static void receipt(string id, int deviceId, Envelope envelope)
{
    string sourceId;
    int sourceDeviceId;

    sourceId = envelope->sourceId();
    sourceDeviceId = envelope->sourceDeviceId();
    send(sourceId, sourceDeviceId,
	 ONLINE_REGISTRY->present(sourceId, sourceDeviceId),
	 new Envelope(id, deviceId, RECEIPT, nil, envelope->timestamp(),
		      sourceId, FALSE));
}

void processEnvelopes(string id, int deviceId, object endpoint)
{
    mapping mbox;

    mbox = mboxes[deviceId + id];
    if (sizeof(mbox[QUEUE]) != 0 && !mbox[ENDPOINT]) {
	mbox[ENDPOINT] = endpoint;
	call_out("deliver", 0, id, deviceId);
    }
}
