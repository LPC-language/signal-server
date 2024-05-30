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

# define CAP_ANNOUNCEMENT_GROUP	0
# define CAP_CHANGE_NUMBER	1
# define CAP_GIFT_BADGES	2
# define CAP_PAYMENT_ACTIVATION	3
# define CAP_PNI		4
# define CAP_SENDER_KEY		5
# define CAP_STORAGE		6
# define CAP_STORIES		7
# define CAP_UUID		8

private int id;
private int registrationId;
private int fetchesMessages;
private string authToken;
private string salt;
private string name;
private int cap;		/* capabilities */

/*
 * initialize device
 */
static void create(int capAnnouncementGroup, int capChangeNumber,
		   int capGiftBadges, int capPaymentActivation, int capPni,
		   int capSenderKey, int capStorage, int capStories,
		   int capUuid)
{
    id = 1;
    cap |= capAnnouncementGroup <<	CAP_ANNOUNCEMENT_GROUP;
    cap |= capChangeNumber <<		CAP_CHANGE_NUMBER;
    cap |= capGiftBadges <<		CAP_GIFT_BADGES;
    cap |= capPaymentActivation <<	CAP_PAYMENT_ACTIVATION;
    cap |= capPni <<			CAP_PNI;
    cap |= capSenderKey <<		CAP_SENDER_KEY;
    cap |= capStorage <<		CAP_STORAGE;
    cap |= capStories <<		CAP_STORIES;
    cap |= capUuid <<			CAP_UUID;
}

void setId(int id)
{
    ::id = id;
}

void setRegistrationId(int registrationId)
{
    ::registrationId = registrationId;
}

void setFetchesMessages(int fetchesMessages)
{
    ::fetchesMessages = fetchesMessages;
}

void setAuthTokenHash(string authToken, string salt)
{
    ::authToken = authToken;
    ::salt = salt;
}

void setName(string name)
{
    ::name = name;
}


int id()			{ return id; }
int registrationId()		{ return registrationId; }
int fetchesMessages()		{ return fetchesMessages; }
string *authTokenHash()		{ return ({ authToken, salt }); }
string name()			{ return name; }
int capAnnouncementGroup()	{ return (cap >> CAP_ANNOUNCEMENT_GROUP) & 1; }
int capChangeNumber()		{ return (cap >> CAP_CHANGE_NUMBER) & 1; }
int capGiftBadges()		{ return (cap >> CAP_GIFT_BADGES) & 1; }
int capPaymentActivation()	{ return (cap >> CAP_PAYMENT_ACTIVATION) & 1; }
int capPni()			{ return (cap >> CAP_PNI) & 1; }
int capSenderKey()		{ return (cap >> CAP_SENDER_KEY) & 1; }
int capStorage()		{ return (cap >> CAP_STORAGE) & 1; }
int capStories()		{ return (cap >> CAP_STORIES) & 1; }
int capUuid()			{ return (cap >> CAP_UUID) & 1; }
