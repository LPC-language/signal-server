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
# include "protocol.h"
# include "params.h"


private RistrettoPoint publicKey;
private RistrettoPoint D1, D2, E1, E2;
private string proof;

/*
 * initialize ProfileKeyCredentialRequest
 */
static void create(ProfileKeyCommitment commitment, RistrettoPoint publicKey,
		   RistrettoPoint D1, RistrettoPoint D2, RistrettoPoint E1,
		   RistrettoPoint E2, string proof)
{
    Statement stmt;
    RistrettoPoint G_j1, G_j2, G_j3;
    mapping pointArgs;

    stmt = new Statement;
    stmt->add("Y",
	      "y", "G");
    stmt->add("D1",
	      "r1", "G");
    stmt->add("E1",
	      "r2", "G");
    stmt->add("J3",
	      "j3", "G_j3");
    stmt->add("D2-J1",
	      "r1", "Y",
	      "j3", "-G_j1");
    stmt->add("E2-J2",
	      "r2", "Y",
	      "j3", "-G_j2");

    ({ G_j1, G_j2, G_j3 }) = PARAMS->profileKeyCommitmentParams();

    pointArgs = ([
	"Y" : publicKey,
	"D1" : D1,
	"E1" : E1,
	"J3" : commitment->J3(),
	"G_j3" : G_j3,
	"D2-J1" : D2 - commitment->J1(),
	"-G_j1" : -G_j1,
	"E2-J2" : E2 - commitment->J2(),
	"-G_j2" : -G_j2
    ]);
    if (!stmt->verify(proof, pointArgs, "")) {
	error("Verification failed");
    }

    ::publicKey = publicKey;
    ::D1 = D1;
    ::D2 = D2;
    ::E1 = E1;
    ::E2 = E2;
    ::proof = proof;
}

/*
 * export as a blob
 */
string transport()
{
    return "\0" + publicKey->bytes() +
	   D1->bytes() + D2->bytes() + E1->bytes() + E2->bytes() +
	   "\xa0\0\0\0\0\0\0\0" + proof;
}


RistrettoPoint publicKey()	{ return publicKey; }
RistrettoPoint *cipherText()	{ return ({ D1, D2, E1, E2 }); }
string proof()			{ return proof; }
