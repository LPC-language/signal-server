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

# include "zkp.h"
# include "params.h"
# include "Sho.h"
# include "credentials.h"

private inherit "~TLS/api/lib/hkdf";
private inherit base64 "/lib/util/base64";
private inherit hex "/lib/util/hex";


# define CREDENTIALS_SEED	"~/config/credentials_seed"
# define SERVER_PUBLIC_KEYS	"~/config/ZKGROUP_SERVER_PUBLIC_PARAMS"

string userDeriveKey;			/* 32 bytes */
string key;
KeyPair authCredentialKey;
Scalar signingKey;
RistrettoPoint publicKey;
KeyPair receiptCredentialKey;
KeyPair profileKeyCredentialKey;
KeyPair authCredentialWithPniKey;

/*
 * get public key from KeyPair
 */
private string *getPubKey(KeyPair key)
{
    return ({ key->C_W()->bytes(), key->I()->bytes() });
}

/*
 * credentials server
 */
static void create()
{
    Sho sho;
    string generator;
    KeyPair unused1Key, unused2Key;
    string *keys;

    /* XXX */
    userDeriveKey = secure_random(32);
    key = secure_random(32);

    sho = PARAMS->serverSecretSho();
    generator = read_file(CREDENTIALS_SEED);
    if (!generator) {
	generator = secure_random(32);
	write_file(CREDENTIALS_SEED, generator);
    }
    sho->absorb(generator);
    sho->ratchet();

    authCredentialKey = new KeyPair(sho, 4, 3);
    unused1Key = new KeyPair(sho, 4, 4);
    signingKey = sho->getScalar();
    publicKey = new RistrettoPoint(ristretto255_basepoint()) * signingKey;
    receiptCredentialKey = new KeyPair(sho, 4, 2);
    unused2Key = new KeyPair(sho, 6);
    profileKeyCredentialKey = new KeyPair(sho, 5);
    authCredentialWithPniKey = new KeyPair(sho, 5);

    keys = getPubKey(authCredentialKey) +
	   getPubKey(unused1Key) +
	   ({ publicKey->bytes() }) +
	   getPubKey(receiptCredentialKey) +
	   getPubKey(unused2Key) +
	   getPubKey(profileKeyCredentialKey) +
	   getPubKey(authCredentialKey);
    remove_file(SERVER_PUBLIC_KEYS);
    write_file(SERVER_PUBLIC_KEYS, base64::encode("\0" + implode(keys, "")));
}

void initialize(string userDeriveKey, string key)
{
    ::userDeriveKey = userDeriveKey;
    ::key = key;
}

string *generate(string id, int derive, int truncate, int prepend)
{
    int time;
    string data, signature;

    if (derive) {
	id = hex::format(HMAC(userDeriveKey, id, "SHA256")[.. 9]);
    }
    time = time();
    data = id + ":" + time;
    signature = HMAC(key, data, "SHA256");
    if (truncate) {
	signature = signature[.. 9];
    }
    return ({ id, ((prepend) ? data : time) + ":" + hex::format(signature) });
}


KeyPair authCredentialKey()		{ return authCredentialKey; }
KeyPair receiptCredentialKey()		{ return receiptCredentialKey; }
KeyPair profileKeyCredentialKey()	{ return profileKeyCredentialKey; }
KeyPair authCredentialWithPniKey()	{ return authCredentialWithPniKey; }
