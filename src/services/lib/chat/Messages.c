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

# ifdef REGISTER

register(CHAT_SERVER, "PUT", "/v1/messages/{}",
	 "putMessages", argHeaderAuth(), argHeader("Unidentified-Access-Key"),
	 argEntityJson());

# else

# include <String.h>
# include "~HTTP/HttpResponse.h"
# include "Timestamp.h"
# include "rest.h"
# include "account.h"
# include "messages.h"
# include "fcm.h"

inherit RestServer;
private inherit base64 "/lib/util/base64";
private inherit uuid "~/lib/uuid";


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

    destinationId = account->id();
    online = ([ ]);
    for (size = sizeof(messages), i = 0; i < size; i++) {
	message = messages[i];
	deviceId = message["destinationDeviceId"];
	endpoint = online[deviceId];
	if (!endpoint) {
	    endpoint = ONLINE_REGISTRY->present(destinationId, deviceId);
	    if (endpoint) {
		online[deviceId] = endpoint;
	    } else {
		FCM_RELAY->sendNotification(account->device(deviceId)->gcmId(),
					    urgent);
	    }
	}
	content = base64::decode(message["content"]);
	MESSAGE_SERVER->send(destinationId, deviceId, endpoint,
			     new Envelope(sourceId, sourceDeviceId,
					  message["type"], new String(content),
					  timestamp, destinationId, urgent));
    }

    respondJson(context, HTTP_OK, ([ "needsSync" : FALSE ]));
}

# endif
