# include <Continuation.h>


object sender;	/* XXX more senders probably needed */

/*
 * initialize FCM relay
 */
static void create()
{
    sender = clone_object("~/obj/fcm_sender");
}

/*
 * send a notification
 */
void sendNotification(string target, int urgent)
{
    sender->notify(target, "notification", "", urgent);
}

/*
 * send a challenge
 */
void sendChallenge(string target, string challenge, Continuation callback)
{
    sender->notify(target, "challenge", challenge, TRUE, callback);
}

/*
 * send a rateLimitChallenge
 */
void sendRateLimitChallenge(string target, string challenge,
			    Continuation callback)
{
    sender->notify(target, "rateLimitChallenge", challenge, TRUE, callback);
}

/*
 * send an attemptLoginContext
 */
void sendAttemptLoginContext(string target, string context)
{
    sender->notify(target, "attemptLoginContext", context, TRUE);
}
