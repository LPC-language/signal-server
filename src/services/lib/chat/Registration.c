/*
 * This file is part of https://github.com/LPC-language/signal-server
 * Copyright (C) 2024-2025 Dworkin B.V.  All rights reserved.
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

register(CHAT_SERVER, "POST", "/v1/verification/session",
	 "postVerificationNewSession", argEntityJson());
register(CHAT_SERVER, "PATCH", "/v1/verification/session/{}",
	 "patchVerificationSession", argEntityJson());
register(CHAT_SERVER, "GET", "/v1/verification/session/{}",
	 "getVerificationSession");
register(CHAT_SERVER, "POST", "/v1/verification/session/{}/code",
	 "postVerificationSessionCode", argHeader("Accept-Language"),
	 argEntityJson());
register(CHAT_SERVER, "PUT", "/v1/verification/session/{}/code",
	 "putVerificationSessionCode", argEntityJson());
register(CHAT_SERVER, "POST", "/v1/registration",
	 "postRegistration", argHeader("Authorization"),
	 argHeader("X-Signal-Agent"), argEntityJson());

# else

# include <String.h>
# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "~HTTP/HttpField.h"
# include "rest.h"
# include "registration.h"
# include "account.h"
# include "fcm.h"

inherit RestServer;
private inherit "/lib/util/random";
private inherit "/lib/util/ascii";
private inherit base64 "/lib/util/base64";
private inherit hex "/lib/util/hex";
private inherit uuid "~/lib/uuid";


/*
 * verification response
 */
private int respondVerification(string context, mapping session,
				varargs int code)
{
    mapping entity;

    entity = ([
	"id": session["id"],
	"allowedToRequestCode": !!session["acceptCode"],
	"requestedInformation": (session["challenge"]) ?
				 ({ "pushChallenge" }) : ({ }),
	"verified": !!session["verified"]
    ]);
    if (session["nextCode"]) {
	entity["nextVerificationAttempt"] = 0;
    }

    return respondJson(context, (code) ? code : HTTP_OK, entity);
}

/*
 * start a new verification session
 */
static int postVerificationNewSession(string context, mapping entity)
{
    string sessionId, pushToken, challenge;
    mapping session;

    ({
	sessionId,
	session
    }) = REGISTRATION_SERVER->getSessionId(entity["number"]);
    if (session["verified"]) {
	return respondVerification(context, session);
    }

    pushToken = entity["pushToken"];
    if (!pushToken || entity["pushTokenType"] != "fcm") {
	return respondVerification(context, session,
				   HTTP_UNPROCESSABLE_CONTENT);
    }
    session["pushToken"] = pushToken;
    if (!session["challenge"]) {
	challenge = hex::format(random_string(16));
	FCM_RELAY->sendChallenge(pushToken, challenge,
				 new Continuation("fcmChallengeSent", context,
						  sessionId, challenge));
	return 0;
    }

    return respondVerification(context, session);
}

/*
 * respond after a FCM challenge was sent
 */
static void fcmChallengeSent(string context, string sessionId, string challenge,
			     int code)
{
    mapping session;

    session = REGISTRATION_SERVER->getSession(sessionId);
    if (code == HTTP_OK) {
	session["challenge"] = challenge;
    }
    respondVerification(context, session, code);
}

/*
 * receive FCM challenge response
 */
static int patchVerificationSession(string context, string sessionId,
				    mapping entity)
{
    mapping session;
    string challenge;

    session = REGISTRATION_SERVER->getSession(sessionId);
    if (!session) {
	return respond(context, HTTP_NOT_FOUND, nil, nil);
    }

    challenge = entity["pushChallenge"];
    if (challenge && challenge == session["challenge"]) {
	session["challenge"] = nil;
	session["acceptCode"] = TRUE;
    }
    return respondVerification(context, session);
}

/*
 * return current verification status
 */
static int getVerificationSession(string context, string sessionId)
{
    mapping session;

    session = REGISTRATION_SERVER->getSession(sessionId);
    if (!session) {
	return respond(context, HTTP_NOT_FOUND, nil, nil);
    }

    return respondVerification(context,
			       REGISTRATION_SERVER->getSession(sessionId));
}

/*
 * receive request to send SMS code
 */
static int postVerificationSessionCode(string context, string sessionId,
				       string acceptLanguage, mapping entity)
{
    mapping session;

    session = REGISTRATION_SERVER->getSession(sessionId);
    if (!session) {
	return respond(context, HTTP_NOT_FOUND, nil, nil);
    }

    /*
     * XXX don't actually send any code
     */
    session["nextCode"] = TRUE;
    return respondVerification(context, session);
}

/*
 * receive SMS code response
 */
static int putVerificationSessionCode(string context, string sessionId,
				      mapping entity)
{
    mapping session;

    session = REGISTRATION_SERVER->getSession(sessionId);
    if (!session) {
	return respond(context, HTTP_NOT_FOUND, nil, nil);
    }

    /*
     * XXX accept any code
     */
    session["acceptCode"] = nil;
    session["nextCode"] = nil;
    session["verified"] = TRUE;
    return respondVerification(context, session);
}

/*
 * register an account
 */
static int postRegistration(string context, HttpAuthentication authorization,
			    string agent, mapping entity)
{
    string phoneNumber, password;

    if (!authorization || lower_case(authorization->scheme()) != "basic") {
	return respond(context, HTTP_BAD_REQUEST, nil, nil);
    }
    sscanf(base64::decode(authorization->authentication()), "%s:%s",
	   phoneNumber, password);

    /* XXX check that sessionId passed verification */

    new Continuation("postRegistration2", phoneNumber, password, agent,
		     entity["accountAttributes"])
	->chain("postRegistration3")
	->chain("postRegistration4", context)
	->runNext();
}

/*
 * register an account, part 2
 */
static Account postRegistration2(string phoneNumber, string password,
				 string agent, mapping attr)
{
    mapping cap;
    Device device;
    Account account;

    cap = attr["capabilities"];
    device = new Device(1, password);
    device->update(attr["name"], attr["registrationId"], agent,
		   attr["fetchesMessages"], cap["announcementGroup"],
		   cap["changeNumber"], cap["giftBadges"],
		   cap["paymentActivation"], cap["pni"], cap["senderKey"],
		   cap["storage"], cap["stories"], cap["uuid"]);
    account = new Account(phoneNumber, PNI_SERVER->getId(phoneNumber), device);
    account->update(attr["discoverableByPhoneNumber"], attr["pin"],
		    attr["pniRegistrationId"], attr["recoveryPassword"],
		    attr["registrationLock"], attr["signalingKey"],
		    base64::decode(attr["unidentifiedAccessKey"]),
		    attr["unrestrictedUnidentifiedAccess"], attr["video"],
		    attr["voice"]);

    return account;
}

/*
 * register an account, part 3
 */
static Account postRegistration3(Account account)
{
    ACCOUNT_SERVER->add(account);

    return account;
}

/*
 * register an account, part 4
 */
static void postRegistration4(string context, Account account)
{
    respondJson(context, HTTP_OK, ([
	"uuid" : uuid::encode(account->id()),
	"number" : account->phoneNumber(),
	"pni" : uuid::encode(account->pni()),
	/* userNameHash : null */
	"storageCapable" : account->device(1)->capStorage()
    ]));
}

# endif
