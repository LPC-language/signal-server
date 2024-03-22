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
void sendChallenge(string target, string challenge)
{
    sender->notify(target, "challenge", challenge, TRUE);
}

/*
 * send a rateLimitChallenge
 */
void sendRateLimitChallenge(string target, string challenge)
{
    sender->notify(target, "rateLimitChallenge", challenge, TRUE);
}

/*
 * send an attemptLoginContext
 */
void sendAttemptLoginContext(string target, string context)
{
    sender->notify(target, "attemptLoginContext", context, TRUE);
}
