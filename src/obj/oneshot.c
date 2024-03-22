# include <String.h>
# include <Continuation.h>
# include "~HTTP/HttpResponse.h"
# include "rest.h"

inherit RestClient;


string address;			/* request address */
string path;			/* request path */
StringBuffer entity;		/* request entity */
mapping headers;		/* request extra headers */
Continuation callback;		/* result callback */

/*
 * initialize POST request connection
 */
static void create(string address, int port, string path, StringBuffer entity,
		   mapping headers, Continuation callback)
{
    ::create(address, port, "created");
    ::address = address;
    ::path = path;
    ::entity = entity;
    ::headers = headers;
    ::callback = callback;
}

/*
 * send POST request
 */
static void created(int code)
{
    if (code == HTTP_OK) {
	request("POST", address, path, "application/json", entity, headers,
		"response", argEntity());
    } else {
	/* connection failed */
	callback->runNext(code, nil);
	destruct_object(this_object());
    }
}

/*
 * callback with response
 */
static void response(int code, StringBuffer entity)
{
    callback->runNext(code, entity);
    callback = nil;
    doneResponse();
    disconnect();
}

/*
 * close connection
 */
static void close()
{
    if (callback) {
	callback->runNext(-1, nil);
    }
    ::close();
}
