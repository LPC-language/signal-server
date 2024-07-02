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
# include "~HTTP/HttpResponse.h"
# include "~TLS/x509.h"
# include "rest.h"

inherit rest RestClient;
private inherit "~/lib/rs256";
private inherit base64 "/lib/util/base64";
private inherit json "/lib/util/json";


# define ATTEMPTS	3	/* number of times to try */

object tokenClient;	/* obtain a firebase access token */
string accessToken;	/* firebase access token */
mixed **queue;		/* message queue */
int active;		/* active connection */
int requested;		/* awaiting response */

/* from service-account.json */
string projectId, keyId, clientEmail, tokenUri;
X509Key key;

/*
 * initialize FCM sender object
 */
static void create()
{
    string str;
    mapping map;

    /*
     * read service-account.json
     */
    str = read_file("~/config/fcm-key/service-account.json");
    if (!str) {
	error("Missing service-account.json");
    }
    map = json::decode(str);
    if (map["type"] != "service_account") {
	error("Not a service account");
    }
    projectId = map["project_id"];
    keyId = map["private_key_id"];
    if (sscanf(map["private_key"],
	       "-----BEGIN PRIVATE KEY-----\n%s\n" +
	       "-----END PRIVATE KEY-----\n",
	       str) == 0) {
	error("Not a PEM key");
    }
    key = new X509Key(base64::decode(implode(explode(str, "\n"), "")));
    clientEmail = map["client_email"];
    tokenUri = map["token_uri"];

    queue = ({ });
}

/*
 * establish connection
 */
static void startConnection()
{
    rest::create("fcm.googleapis.com", 443, "started");
}

/*
 * connection established (or not)
 */
static void started(int code)
{
    if (code == HTTP_OK) {
	active = TRUE;
	call_out("sendMessage", 0);
    } else {
	call_out("startConnection", 1);	/* try again */
    }
}

/*
 * send a message
 */
static void sendMessage()
{
    int attempts, urgent;
    Continuation callback;
    string target, type, message;

    if (!active) {
	startConnection();
    } else {
	({ attempts, callback, target, type, message, urgent }) = queue[0];
	message = "{\"message\":{\"token\":\"" + target + "\"," +
		  "\"data\":{\"" + type + "\":\"" + message + "\"}," +
		  "\"android\":{\"priority\":\"" +
		  ((urgent) ? "high" : "normal") + "\"}}}";
	requested = TRUE;
	request("POST", "fcm.googleapis.com",
		"/v1/projects/" + projectId + "/messages:send",
		"application/json", new StringBuffer(message),
		([ "Authorization" : "Bearer " + accessToken ]),
		"response", argEntity());
    }
}

/*
 * process response
 */
static void response(int code, StringBuffer entity)
{
    Continuation callback;

    requested = FALSE;
    switch (code) {
    case HTTP_NOT_FOUND:
	/*
	 * Firebase sometimes responds "Requested entity was not found",
	 * which presumably refers to the token.  Try a few times.
	 */
	if (--queue[0][0] > 0) {
	    call_out("sendMessage", 0);
	    break;
	}
	/* fall through */
    case HTTP_OK:
	callback = queue[0][1];
	if (callback) {
	    callback->runNext(code);
	}
	queue = queue[1 ..];
	if (sizeof(queue) != 0) {
	    call_out("sendMessage", 0);
	}
	break;

    case HTTP_UNAUTHORIZED:
	call_out("getAccessToken", 0);
	break;

    default:
	call_out("sendMessage", 1);	/* again */
	break;
    }
    doneResponse();
}

/*
 * connection closed
 */
static void close()
{
    active = FALSE;
    if (requested) {
	requested = FALSE;
	call_out("startConnection", 0);
    }
}

/*
 * obtain new access token
 */
static void getAccessToken()
{
    int time;
    string str;

    /*
     * create JWT
     */
    time = time();
    str = "https://www.googleapis.com/auth/firebase.messaging";
    str = base64::urlEncode("{\"typ\": \"JWT\", " +
			    "\"alg\": \"RS256\", " +
			    "\"kid\": \"" + keyId + "\"}") +
	  "." +
	  base64::urlEncode("{\"iat\": " + time +
			    ", \"exp\": " + (time + 3600) +
			    ", \"iss\": \"" + clientEmail +
			    "\", \"aud\": \"" + tokenUri +
			    "\", \"scope\": \"" + str + "\"}");
    str += "." + base64::urlEncode(rs256Sign(str, key));

    str = "{\"assertion\": \"" + str + "\", " +
	  "\"grant_type\": \"urn:ietf:params:oauth:grant-type:jwt-bearer\"}";
    tokenClient = clone_object("~/obj/oneshot", "oauth2.googleapis.com", 443,
			       "/token", new StringBuffer(str), ([
	"x-goog-api-client" :
	"gl-python/3.10.12 auth/2.28.1 auth-request-type/at cred-type/sa"
    ]), new Continuation("storeAccessToken"));
}

/*
 * store access token
 */
static void storeAccessToken(int code, StringBuffer entity)
{
    switch (code) {
    case HTTP_OK:
	accessToken = json::decode(entity->chunk())["access_token"];
	sendMessage();
	break;

    case HTTP_BAD_REQUEST:
    case HTTP_UNAUTHORIZED:
	error("Service account not accepted");

    default:
	call_out("getAccessToken", 1); 	/* try again */
	break;
    }
}

/*
 * enqueue message
 */
static void enqueue(string target, string type, string message, int urgent,
		    Continuation callback)
{
    queue += ({ ({ ATTEMPTS, callback, target, type, message, urgent }) });
    if (sizeof(queue) == 1) {
	if (!accessToken) {
	    getAccessToken();
	} else {
	    sendMessage();
	}
    }
}

/*
 * send a notification
 */
void notify(string target, string type, string message, int urgent,
	    varargs Continuation callback)
{
    call_out("enqueue", 0, target, type, message, urgent, callback);
}
