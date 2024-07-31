/*
 * This file is part of https://github.com/LPC-language/signal-server
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
# include "Sho.h"
# include "params.h"
# include "credentials.h"
# include "protocol.h"

private inherit asn "/lib/util/asn";
private inherit uuid "~/lib/uuid";


private Scalar t;
private RistrettoPoint U;
private RistrettoPoint S1;
private RistrettoPoint S2;
private int expirationTime;
private string proof;

/*
 * time in seconds encoded in 8 bytes, big endian
 */
private string timeBytes(int time)
{
    return asn::unsignedExtend(asn::extend(asn::encode(time), 4), 8);
}

/*
 * get a scalar from a time
 */
private Scalar timeScalar(int time)
{
    Sho sho;

    sho = PARAMS->timeSho();
    sho->absorb(timeBytes(time));
    sho->ratchet();
    return sho->getScalar();
}

/*
 * initialize (Expiring)ProfileKeyCredentialResponse
 */
static void create(ProfileKeyCredentialRequest request, string uuid,
		   int expirationTime, string random)
{
    RistrettoPoint *M, publicKey, D1, D2, E1, E2, Vprime, M5, R1, R2;
    Sho sho;
    KeyPair key;
    Scalar rprime;
    RistrettoPoint *params;
    Statement stmt;
    mapping scalarArgs, pointArgs;

    M = uuid::points(uuid);
    publicKey = request->publicKey();
    ({ D1, D2, E1, E2 }) = request->cipherText();
    ::expirationTime = expirationTime;
    key = CREDENTIALS_SERVER->profileKeyCredentialKey();

    sho = PARAMS->profileKeyCredentialSho();
    sho->absorb(random);
    sho->ratchet();
    ({ t, U, Vprime }) = key->prepare(sho, M);
    rprime = sho->getScalar();

    params = PARAMS->credentialParams();
    M5 = params[CRED_G_m5] * timeScalar(expirationTime);

    R1 = new RistrettoPoint(ristretto255_basepoint()) * rprime;
    R2 = publicKey * rprime + Vprime + M5 * key->y()[4];
    S1 = R1 + D1 * key->y()[2] + E1 * key->y()[3];
    S2 = R2 + D2 * key->y()[2] + E2 * key->y()[3];

    stmt = new Statement;
    stmt->add("C_W",
	      "w", "G_w",
	      "wprime", "G_wprime");
    stmt->add("G_V-I",
	      "x0", "G_x0",
	      "x1", "G_x1",
	      "y1", "G_y1",
	      "y2", "G_y2",
	      "y3", "G_y3",
	      "y4", "G_y4",
	      "y5", "G_y5");
    stmt->add("S1",
	      "y3", "D1",
	      "y4", "E1",
	      "rprime", "G");
    stmt->add("S2",
	      "y3", "D2",
	      "y4", "E2",
	      "rprime", "Y",
	      "w", "G_w",
	      "x0", "U",
	      "x1", "tU",
	      "y1", "M1",
	      "y2", "M2",
	      "y5", "M5");

    scalarArgs = ([
	"w" : key->w(),
	"wprime" : key->wprime(),
	"x0" : key->x0(),
	"x1" : key->x1(),
	"y1" : key->y()[0],
	"y2" : key->y()[1],
	"y3" : key->y()[2],
	"y4" : key->y()[3],
	"y5" : key->y()[4],
	"rprime" : rprime
    ]);
    pointArgs = ([
	"C_W" : key->C_W(),
	"G_w" : params[CRED_G_w],
	"G_wprime" : params[CRED_G_wprime],
	"G_V-I" : params[CRED_G_V] - key->I(),
	"G_x0" : params[CRED_G_x0],
	"G_x1" : params[CRED_G_x1],
	"G_y1" : params[CRED_G_y + 0],
	"G_y2" : params[CRED_G_y + 1],
	"G_y3" : params[CRED_G_y + 2],
	"G_y4" : params[CRED_G_y + 3],
	"G_y5" : params[CRED_G_y + 4],
	"S1" : S1,
	"D1" : D1,
	"E1" : E1,
	"S2" : S2,
	"D2" : D2,
	"E2" : E2,
	"Y" : publicKey,
	"U" : U,
	"tU" : U * t,
	"M1" : M[0],
	"M2" : M[1],
	"M5" : M5
    ]);

    proof = stmt->prove(scalarArgs, pointArgs, "", sho->squeeze(32));
}

/*
 * export as a blob
 */
string transport()
{
    return "\0" + t->bytes() + U->bytes() + S1->bytes() + S2->bytes() +
	   asn::reverse(timeBytes(expirationTime)) +
	   "\x60\1\0\0\0\0\0\0" + proof;
}


Scalar t()		{ return t; }
RistrettoPoint U()	{ return U; }
RistrettoPoint S1()	{ return S1; }
RistrettoPoint S2()	{ return S2; }
int expirationTime()	{ return expirationTime; }
string proof()		{ return proof; }
