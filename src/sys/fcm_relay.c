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
