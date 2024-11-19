/*
 * This file is part of https://github.com/LPC-language/signal-server
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

# ifdef REGISTER

register(CHAT_SERVER, "PUT", "/v1/messages/{}",
	 "putMessages", argHeaderAuth(), argHeader("Unidentified-Access-Key"),
	 argEntityJson());

# else

# include <String.h>
# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "Timestamp.h"
# include "rest.h"
# include "account.h"
# include "messages.h"
# include "fcm.h"
# include <status.h>

inherit RestServer;
private inherit base64 "/lib/util/base64";
private inherit uuid "~/lib/uuid";


# define READY			0
# define AWAIT_RESPONSE		1

# define RECEIPT		5

private object *queue;		/* envelope queue */
private int state;		/* delivery state */

static void chatSendRequest(string verb, string path, StringBuffer body,
			    mapping extraHeaders, Continuation cont,
			    varargs mixed arguments...);

static void putMessages(string context, string uuid, Account account,
			Device device, string accessKey, mapping entity)
{
    string story;

    sscanf(uuid, "%s?story=%s", uuid, story);

    call_out("putMessages2", 0, context,
	     ACCOUNT_SERVER->get(uuid::decode(entity["destination"])),
	     account->id(), device->id(), entity["messages"],
	     new Timestamp(entity["timestamp"]), entity["urgent"]);
}

static void putMessages2(string context, Account account, string sourceId,
			 int sourceDeviceId, mapping *messages,
			 Timestamp timestamp, int urgent)
{
    string destinationId, content;
    int size, i, deviceId;
    mapping online, message;
    object endpoint;
    Envelope envelope;

    destinationId = account->id();
    online = ([ ]);
    for (size = sizeof(messages), i = 0; i < size; i++) {
	message = messages[i];
	envelope = new Envelope(this_object(), sourceId, sourceDeviceId,
				message["type"],
				new String(base64::decode(message["content"])),
				timestamp, destinationId, deviceId, urgent);
	deviceId = message["destinationDeviceId"];
	endpoint = online[deviceId];
	if (!endpoint) {
	    endpoint = ONLINE_REGISTRY->present(destinationId, deviceId);
	    if (endpoint) {
		online[deviceId] = endpoint;
	    } else {
		FCM_RELAY->sendNotification(account->device(deviceId)->gcmId(),
					    urgent);
		call_out_other(MESSAGE_SERVER, "stack", 0, ({ envelope }));
		continue;
	    }
	}

	call_out_other(endpoint, "deliver", 0, ({ envelope }));
    }

    respondJson(context, HTTP_OK, ([ "needsSync" : FALSE ]));
}

/*
 * send one envelope to the client
 */
private void sendEnvelope(Envelope envelope)
{
    chatSendRequest("PUT", "/api/v1/message", envelope->transport(),
    ([
	"X-Signal-Timestamp" : new Timestamp->transport()
    ]), new Continuation("delivered", envelope));
    state = AWAIT_RESPONSE;
}

/*
 * tell the client that the queue is empty for now
 */
private void emptyQueue()
{
    chatSendRequest("PUT", "/api/v1/queue/empty", nil, nil, nil);
}

/*
 * deliver a stack of envelopes one by one
 */
void deliver(Envelope *envelopes)
{
    if (!queue) {
	queue = envelopes;
    } else {
	queue += envelopes;
    }

    if (state == READY) {
	/* send envelope immediately */
	sendEnvelope(queue[0]);
    }
}

/*
 * calback after delivery
 */
static void delivered(Envelope envelope, int code)
{
    object sender;

    if (code == HTTP_OK) {
	queue = queue[1 ..];
	state = READY;

	if (envelope->type() != RECEIPT) {
	    sender = envelope->origin();
	    envelope = new Envelope(this_object(), envelope->destinationId(),
				    envelope->destinationDeviceId(), RECEIPT,
				    nil, envelope->timestamp(),
				    envelope->sourceId(),
				    envelope->sourceDeviceId(), FALSE);
	    if (sender) {
		/* send receipt */
		call_out_other(sender, "deliver", 0, ({ envelope }));
	    } else {
		/* send receipt the circuitrous route */
		call_out_other(MESSAGE_SERVER, "stack", 0, ({ envelope }));
	    }
	}

	/* send next envelope, or EOM */
	if (sizeof(queue) != 0) {
	    sendEnvelope(queue[0]);
	} else {
	    emptyQueue();
	}
    }
}

/*
 * override close() to save message queue
 */
static void close()
{
    object *envelopes;
    mixed **callouts;
    int i, sz;

    envelopes = (queue) ? queue : ({ });
    callouts = status(this_object(), O_CALLOUTS);
    for (i = 0, sz = sizeof(callouts); i < sz; i++) {
	if (callouts[i][CO_FUNCTION] == "deliver") {
	    envelopes += callouts[i][CO_FIRSTXARG];
	}
    }

    if (sizeof(envelopes) != 0) {
	call_out_other(MESSAGE_SERVER, "stack", 0, envelopes);
    }

    ::close();
}

# endif
