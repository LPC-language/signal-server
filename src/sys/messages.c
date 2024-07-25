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
# include "KVstoreExp.h"
# include "Timestamp.h"
# include "account.h"
# include "messages.h"
# include "fcm.h"


# define DURATION	30 * 24 * 3600
# define QUEUE		0
# define ENDPOINT	1

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
atomic void send(Account account, Device device, Envelope envelope)
{
    string index, guid;
    mapping mbox;
    int notify;

    /* add to message queue */
    index = device->id() + account->id();
    mbox = mboxes[index];
    if (!mbox) {
	mboxes[index] = mbox = ([ ]);
	mbox[QUEUE] = ({ });
    }
    guid = envelope->guid();
    messages->add(guid, envelope);
    mbox[QUEUE] += ({ guid });
    call_out("forward", 0, account, device);
}

static void forward(Account account, Device device)
{
    string id;
    int deviceId;
    object endpoint;

    id = account->id();
    deviceId = device->id();
    endpoint = ONLINE_REGISTRY->present(id, deviceId);
    if (endpoint) {
	call_out("sendTo", 0, id, deviceId, endpoint);
    } else {
	FCM_RELAY->sendNotification(device->gcmId(), TRUE);
    }
}

atomic static void sendTo(string id, int deviceId, object endpoint)
{
    mapping mbox;
    string guid;
    Envelope envelope;

    if (endpoint) {
	mbox = mboxes[deviceId + id];
	if (!mbox[ENDPOINT]) {
	    mbox[ENDPOINT] = endpoint;
	    guid = mbox[QUEUE][0];
	    envelope = messages[guid];
	    if (envelope) {
		endpoint->chatSendRequest("PUT", "/api/v1/message",
					  envelope->transport(), ([
		    "X-Signal-Timestamp" : new Timestamp->transport()
		]), new Continuation("callback", guid));
	    }
	}
    }
}

static void callback(string guid, mixed foo...)
{
}

void processMessages(string id, int deviceId, object endpoint)
{
    call_out("sendTo", 0, id, deviceId, endpoint);
}
