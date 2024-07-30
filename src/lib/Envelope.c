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
# include "Timestamp.h"

private inherit "~/lib/proto";
private inherit uuid "~/lib/uuid";


private int type;			/* content type */
private Timestamp timestamp;		/* content timestamp */
private int sourceDeviceId;		/* source device ID */
private String content;			/* encrypted content */
private string guid;			/* message GUID */
private Timestamp serverTimestamp;	/* envelope timestamp */
private string sourceId;		/* source account ID */
private string destinationId;		/* destination account ID */
private int urgent;			/* urgent? */

/*
 * create message envelope
 */
static void create(string sourceId, int sourceDeviceId, int type,
		   String content, Timestamp timestamp, string destinationId,
		   int urgent)
{
    ::type = type;
    ::timestamp = timestamp;
    ::sourceDeviceId = sourceDeviceId;
    ::content = content;
    guid = secure_random(16);
    serverTimestamp = new Timestamp();
    ::sourceId = sourceId;
    ::destinationId = destinationId;
    ::urgent = urgent;
}

/*
 * export envelope as blob
 */
StringBuffer transport()
{
    StringBuffer buffer;

    buffer = new StringBuffer("\010");
    buffer->append(protoInt(type));
    buffer->append("\050");
    buffer->append(protoAsnTime(timestamp->time(), timestamp->mtime()));
    buffer->append("\070");
    buffer->append(protoInt(sourceDeviceId));
    if (content) {
	buffer->append("\102");
	buffer->append(protoStrbuf(content->buffer()));
    }
    buffer->append("\112");
    buffer->append(protoString(uuid::encode(guid)));
    buffer->append("\120");
    buffer->append(protoAsnTime(serverTimestamp->time(),
				serverTimestamp->mtime()));
    buffer->append("\132");
    buffer->append(protoString(uuid::encode(sourceId)));
    buffer->append("\152");
    buffer->append(protoString(uuid::encode(destinationId)));
    buffer->append("\160");
    buffer->append(protoInt(urgent));

    return buffer;
}


int type()			{ return type; }
string sourceId()		{ return sourceId; }
int sourceDeviceId()		{ return sourceDeviceId; }
Timestamp timestamp()		{ return timestamp; }
String content()		{ return content; }
string guid()			{ return guid; }
Timestamp serverTimestamp()	{ return serverTimestamp; }
string destinationId()		{ return destinationId; }
int urgent()			{ return urgent; }
