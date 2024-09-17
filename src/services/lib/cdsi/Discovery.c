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

register(CDSI_SERVER, "GET", "/v1/{}/discovery",
	 "getDiscovery", argHeader("Authorization"), argHeader("Upgrade"),
	 argHeader("Connection"), argHeader("Sec-WebSocket-Key"),
	 argHeader("Sec-WebSocket-Version"));

# else

# include <String.h>
# include "~HTTP/HttpConnection.h"
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "rest.h"
# include "credentials.h"
# include "account.h"

inherit RestServer;
private inherit "/lib/util/ascii";
private inherit base64 "/lib/util/base64";
private inherit "~/lib/proto";
private inherit "~/lib/phone";


# define STATE_INIT		0
# define STATE_HANDSHAKE	1
# define STATE_RECEIVE		2
# define STATE_RECEIVE_ACK	3
# define STATE_DONE		4

# define BATCH_SIZE		5

# define CLOSE_ILLEGAL_ARG	"\x0f\xa3"
# define CLOSE_FAILURE		"\x0f\xae"

private int state;		/* current state */
private string *numbers;	/* phone numbers in query */

/*
 * CDSI discovery
 */
static int getDiscovery(string context, string enclave, HttpAuthentication auth,
			string upgrade, string connection, string key,
			string version)
{
    string user, credential;

    if (!auth || lower_case(auth->scheme()) != "basic" ||
	sscanf(base64::decode(auth->authentication()), "%s:%s", user,
	       credential) != 2 ||
	!CREDENTIALS_SERVER->verify(user, credential, TRUE, FALSE)) {
	return respond(context, HTTP_UNAUTHORIZED, nil, nil);
    }

    if (upgrade == "websocket" && connection == "Upgrade" && key &&
	version == "13") {
	call_out("attest", 0);
	return upgradeToWebSocket("cdsi", key);
    } else {
	return respond(context, HTTP_BAD_REQUEST, nil, nil);
    }
}

/*
 * fake attestation
 */
static void attest()
{
    sendChunk(new StringBuffer("CDSI"));
    state = STATE_HANDSHAKE;
}

/*
 * get phone numbers in 8-byte chunks from a string buffer
 */
private string *parsePhoneNumbers(StringBuffer chunk)
{
    int len, i, offset;
    string *numbers, buf, str;

    len = chunk->length();
    if (len & 0x07) {
	error("Bad phone number list");
    }
    len >>= 3;

    numbers = allocate(len);
    for (i = 0; i < len; i++) {
	({ str, buf, offset }) = parseBytes(chunk, buf, offset, 8);
	numbers[i] = str;
    }

    return numbers;
}

/*
 * parse a client request
 */
private string *parseClientRequest(StringBuffer chunk)
{
    int c, offset, tokenAck, aciWithoutUak;
    string buf, token;
    StringBuffer aciUaks, prevE164s, newE164s, discardE164s;

    ({ c, buf, offset }) = parseByte(chunk, nil, 0);
    if (c == 012) {
	({ aciUaks, buf, offset }) = parseStrbuf(chunk, buf, offset);
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
    }
    if (c == 022) {
	({ prevE164s, buf, offset }) = parseStrbuf(chunk, buf, offset);
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
    } else {
	prevE164s = new StringBuffer;
    }
    if (c == 032) {
	({ newE164s, buf, offset }) = parseStrbuf(chunk, buf, offset);
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
    } else {
	newE164s = new StringBuffer;
    }
    if (c == 042) {
	({ discardE164s, buf, offset }) = parseStrbuf(chunk, buf, offset);
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
    }
    if (c == 062) {
	({ token, buf, offset }) = parseString(chunk, buf, offset);
	({ c, buf, offset }) = parseByte(chunk, buf, offset);
    }
    if (c == 070) {
	({ tokenAck, buf, offset }) = parseInt(chunk, buf, offset);
	return nil;
    }
    if (c == 100) {
	({ aciWithoutUak, buf, offset }) = parseInt(chunk, buf, offset);
    }

    return parsePhoneNumbers(prevE164s) + parsePhoneNumbers(newE164s);
}

/*
 * send a token in a ClientResponse
 */
private void sendTokenResponse()
{
    sendChunk(new StringBuffer("\32" + protoString("OK")));
}

/*
 * send a ClientResponse with number + pni + uuid triples
 */
private void sendClientResponse(string *results)
{
    StringBuffer triples, response;
    int i, size;

    triples = new StringBuffer;
    for (i = 0, size = sizeof(results); i + 1638 < size; i += 1638) {
	triples->append(implode(results[i .. i + 1637], ""));
    }
    triples->append(implode(results[i ..], ""));
    response = new StringBuffer("\12");
    response->append(protoStrbuf(triples));
    response->append("\40" + protoInt(0));

    sendChunk(response);
}

/*
 * check a batch of phone numbers
 */
static void cdsiBatch(string *numbers, string *results, int offset)
{
    int size, i;
    Account account;

    size = sizeof(numbers);
    if (offset + BATCH_SIZE < size) {
	size = offset + BATCH_SIZE;
    }
    for (i = offset; i < size; i++) {
	account = ACCOUNT_SERVER->getByNumber(numToPhone(numbers[i]));
	results[i] = numbers[i] + ((account) ?
				    account->pni() + account->id() :
				    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" +
				    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
    }

    if (i < sizeof(numbers)) {
	call_out("cdsiBatch", 0, numbers, results, i);
    } else {
	sendClientResponse(results);
	sendClose(WEBSOCK_STATUS_1000 + "OK");
    }
}

/*
 * receive a WebSocket command
 */
static void cdsiReceiveChunk(StringBuffer chunk)
{
    switch (state) {
    case STATE_INIT:
	sendClose(CLOSE_ILLEGAL_ARG);
	state = STATE_DONE;
	break;

    case STATE_HANDSHAKE:
	if (chunk->chunk() == "Client") {
	    sendChunk(new StringBuffer("Ready"));
	    state = STATE_RECEIVE;
	} else {
	    sendClose(CLOSE_ILLEGAL_ARG);
	    state = STATE_DONE;
	}
	break;

    case STATE_RECEIVE:
	try {
	    numbers = parseClientRequest(chunk);
	    if (numbers) {
		sendTokenResponse();
		state = STATE_RECEIVE_ACK;
	    } else {
		sendClose(CLOSE_ILLEGAL_ARG);
		state = STATE_DONE;
	    }
	} catch (...) {
	    sendClose(CLOSE_FAILURE);
	    state = STATE_DONE;
	}
	break;

    case STATE_RECEIVE_ACK:
	try {
	    if (!parseClientRequest(chunk)) {
		call_out("cdsiBatch", 0, numbers, allocate(sizeof(numbers)), 0);
	    }
	} catch (...) {
	    sendClose(CLOSE_FAILURE);
	}
	state = STATE_DONE;
	break;

    case STATE_DONE:
	sendClose(CLOSE_ILLEGAL_ARG);
	break;
    }
}

# endif
