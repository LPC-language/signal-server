# include "KVstoreExp.h"
# include "services.h"

private inherit "/lib/util/random";
private inherit hex "/lib/util/hex";


# define DURATION	30 * 24 * 3600

object sessions;	/* session mappings */
object phoneIndex;	/* phoneNumber : sessionId */

/*
 * initialize registration server
 */
static void create()
{
    sessions = new KVstoreExp(100, DURATION);
    phoneIndex = new KVstoreExp(100, DURATION);
}

/*
 * get session ID and session, creating a new session if needed
 */
mixed *getSessionId(string phoneNumber)
{
    if (previous_program() == VERIFICATION_SERVICE) {
	string sessionId;
	mapping session;

	sessionId = phoneIndex[phoneNumber];
	if (sessionId) {
	    return ({ sessionId, sessions[sessionId] });
	}

	session = ([ "phoneNumber" : phoneNumber ]);

	for (;;) {
	    sessionId = hex::format(random_string(16));
	    try {
		sessions->add(sessionId, session);
	    } catch (...) {
		continue;
	    }
	    session["id"] = sessionId;
	    return ({ phoneIndex[phoneNumber] = sessionId, session });
	}
    }
}

/*
 * get session by sessionId
 */
mapping getSession(string sessionId)
{
    if (previous_program() == VERIFICATION_SERVICE) {
	return sessions[sessionId];
    }
}

/*
 * remove session
 */
void remove(string sessionId)
{
    if (previous_program() == VERIFICATION_SERVICE) {
	mapping values;

	values = sessions[sessionId];
	if (values) {
	    phoneIndex[values["phoneNumber"]] = nil;
	    sessions[sessionId] = nil;
	}
    }
}
