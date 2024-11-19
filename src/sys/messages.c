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

object messages;	/* messages */
object mboxes;		/* mail boxes */

/*
 * initialize message server
 */
static void create()
{
    messages = new KVstoreExp(199, DURATION);
    mboxes = new KVstore(194);
}

/*
 * keep a stack of envelopes to send later
 */
atomic void stack(Envelope *envelopes)
{
    Envelope envelope;
    string index, guid;
    mapping mbox;
    int i, sz;

    /* add to message queue */
    envelope = envelopes[0];
    index = envelope->destinationDeviceId() + envelope->destinationId();
    mbox = mboxes[index];
    if (!mbox) {
	mboxes[index] = mbox = ([ QUEUE : ({ }) ]);
    }
    for (i = 0, sz = sizeof(envelopes); i < sz; i++) {
	envelope = envelopes[i];
	guid = envelope->guid();
	messages->add(guid, envelope);
	mbox[QUEUE] += ({ guid });
    }
}

/*
 * send a stack of envelopes
 */
atomic void send(string accountId, int deviceId, object endpoint)
{
    mapping mbox;
    string *queue, guid;
    Envelope *envelopes, envelope;
    int i, sz;

    if (endpoint) {
	mbox = mboxes[deviceId + accountId];
	if (mbox) {
	    queue = mbox[QUEUE];
	    if (queue && sizeof(queue) != 0) {
		mbox[QUEUE] = ({ });
		envelopes = ({ });

		for (i = 0, sz = sizeof(queue); i < sz; i++) {
		    guid = queue[i];
		    envelope = messages[guid];
		    if (envelope) {
			envelopes += ({ envelope });
		    }
		}

		if (sizeof(envelopes) != 0) {
		    call_out_other(endpoint, "deliver", 0, envelopes);
		}
	    }
	}
    }
}
