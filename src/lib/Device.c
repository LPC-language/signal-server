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

# include "Timestamp.h"

private inherit "hash";


# define FETCHES_MESSAGES	0
# define ANNOUNCEMENT_GROUP	1
# define CHANGE_NUMBER		2
# define GIFT_BADGES		3
# define PAYMENT_ACTIVATION	4
# define PNI			5
# define SENDER_KEY		6
# define STORAGE		7
# define STORIES		8
# define UUID			9

private int id;
private int registrationId;
private string authToken;
private string salt;
private string name;
private string agent;
private int pkId;
private string preKey;
private string pkSignature;
private int pniPkId;
private string pniPreKey;
private string pniPkSignature;
private string gcmId;
private int cap;		/* capabilities */
private Timestamp created;
private Timestamp lastSeen;

void setLastSeen()
{
    if (!created) {
	created = new Timestamp;
    }
    lastSeen = new Timestamp(time(), TRUE);
}

/*
 * initialize device
 */
static void create(int id, string password)
{
    ::id = id;
    ({ authToken, salt }) = hash(password);
    setLastSeen();
}

void updateCapabilities(int announcementGroup, int changeNumber, int giftBadges,
			int paymentActivation, int pni, int senderKey,
			int storage, int stories, int uuid)
{
    cap &= TRUE <<		FETCHES_MESSAGES;
    cap |= announcementGroup <<	ANNOUNCEMENT_GROUP;
    cap |= changeNumber <<	CHANGE_NUMBER;
    cap |= giftBadges <<	GIFT_BADGES;
    cap |= paymentActivation <<	PAYMENT_ACTIVATION;
    cap |= pni <<		PNI;
    cap |= senderKey <<		SENDER_KEY;
    cap |= storage <<		STORAGE;
    cap |= stories <<		STORIES;
    cap |= uuid <<		UUID;
}

void update(string name, int registrationId, string agent, int fetchesMessages,
	    int capAnnouncementGroup, int capChangeNumber, int capGiftBadges,
	    int capPaymentActivation, int capPni, int capSenderKey,
	    int capStorage, int capStories, int capUuid)
{
    ::name = name;
    ::registrationId = registrationId;
    ::agent = agent;
    cap = fetchesMessages << FETCHES_MESSAGES;
    updateCapabilities(capAnnouncementGroup, capChangeNumber, capGiftBadges,
		       capPaymentActivation, capPni, capSenderKey, capStorage,
		       capStories, capUuid);
}

int verifyPassword(string password)
{
    setLastSeen();
    return (authToken == hash(password, salt)[0]);
}

void setRegistrationId(int registrationId)
{
    ::registrationId = registrationId;
}

void setFetchesMessages(int fetchesMessages)
{
    cap = cap & ~(1 << FETCHES_MESSAGES) |
	  (fetchesMessages << FETCHES_MESSAGES);
}

void setGcmId(string gcmId)
{
    ::gcmId = gcmId;
}

void updateSignedPreKey(int keyId, string key, string signature)
{
    /*
     * avoid object modification if possible
     */
    if (pkId != keyId) {
	pkId = keyId;
    }
    if (preKey != key) {
	preKey = key;
    }
    if (pkSignature != signature) {
	pkSignature = signature;
    }
}

void updateSignedPniPreKey(int keyId, string key, string signature)
{
    /*
     * avoid object modification if possible
     */
    if (pniPkId != keyId) {
	pniPkId = keyId;
    }
    if (pniPreKey != key) {
	pniPreKey = key;
    }
    if (pniPkSignature != signature) {
	pniPkSignature = signature;
    }
}

mixed *signedPreKey()
{
    return ({ pkId, preKey, pkSignature });
}

mixed *signedPniPreKey()
{
    return ({ pniPkId, pniPreKey, pniPkSignature });
}


int id()			{ return id; }
int registrationId()		{ return registrationId; }
string *authTokenHash()		{ return ({ authToken, salt }); }
string name()			{ return name; }
string gcmId()			{ return gcmId; }
int fetchesMessages()		{ return (cap >> FETCHES_MESSAGES) & 1; }
int capAnnouncementGroup()	{ return (cap >> ANNOUNCEMENT_GROUP) & 1; }
int capChangeNumber()		{ return (cap >> CHANGE_NUMBER) & 1; }
int capGiftBadges()		{ return (cap >> GIFT_BADGES) & 1; }
int capPaymentActivation()	{ return (cap >> PAYMENT_ACTIVATION) & 1; }
int capPni()			{ return (cap >> PNI) & 1; }
int capSenderKey()		{ return (cap >> SENDER_KEY) & 1; }
int capStorage()		{ return (cap >> STORAGE) & 1; }
int capStories()		{ return (cap >> STORIES) & 1; }
int capUuid()			{ return (cap >> UUID) & 1; }
Timestamp created()		{ return created; }
Timestamp lastSeen()		{ return lastSeen; }
