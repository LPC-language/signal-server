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

# include "account.h"

inherit "~/lib/proto";
private inherit base64 "/lib/util/base64";
private inherit uuid "~/lib/uuid";


# define CERTIFICATE_DURATION	7 * 24 * 3600	/* 7 days */

string caPubKey, caPrivKey;	/* CA keys */
string serverCertificate;	/* signing certificate */
string serverKey;		/* signing key */

/*
 * initialize certificate server
 */
static void create()
{
    string str;

    ({ caPubKey, caPrivKey }) = encrypt("X25519 key");
    ({ str, serverKey }) = encrypt("X25519 key");
    str = "UDSC" + "\5" + str;
    serverCertificate = "\12" + protoString(str) +
			"\22" + protoString(encrypt("XEd25519 sign", caPrivKey,
						    str));
}

/*
 * generate SenderCertificate
 */
string generate(Account account, int deviceId, varargs string phoneNumber)
{
    int time;
    float mtime;
    string str;

    ({ time, mtime }) = millitime();
    time += CERTIFICATE_DURATION;

    str = "\20" + protoInt(deviceId) +
	  "\31" + protoFixedTime(time, mtime) +
	  "\42" + protoString(base64::decode(account->identityKey())) +
	  "\52" + protoString(serverCertificate) +
	  "\62" + protoString(uuid::encode(account->id()));
    if (phoneNumber) {
	str = "\12" + protoString(phoneNumber) + str;
    }

    return "\12" + protoString(str) +
	   "\22" + protoString(encrypt("XEd25519 sign", serverKey, str));
}
