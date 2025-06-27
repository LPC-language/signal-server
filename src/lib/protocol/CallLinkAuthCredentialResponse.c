/*
 * This file is part of https://github.com/LPC-language/signal-server
 * Copyright (C) 2025 Dworkin B.V.  All rights reserved.
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

private inherit uuid "~/lib/uuid";
private inherit "~/lib/time";


private Scalar t;
private RistrettoPoint U;
private RistrettoPoint V;
private string proof;

/*
 * initialize CallLinkAuthCredentialResponse
 */
static void create(string uuid, int redemptionTime, string random)
{
    RistrettoPoint *M, *params;
    Sho sho;
    KeyPair key;
    Statement stmt;
    mapping scalarArgs, pointArgs;

    sho = PARAMS->callLinkAuthCredentialSho();
    sho->absorb(timeBytes(redemptionTime));
    sho->ratchet();
    M = ({ sho->getPoint() }) + uuid::points(uuid);

    sho = PARAMS->credentialSho();
    sho->absorb(random);
    sho->ratchet();

    key = CREDENTIALS_SERVER->credentialKey();
    ({ t, U, V }) = key->prepare(sho, M);

    stmt = new Statement;
    stmt->add("C_W",
	      "w", "G_w",
	      "wprime", "G_wprime");
    stmt->add("G_V-I",
	      "x0", "G_x0",
	      "x1", "G_x1",
	      "y0", "G_y0",
	      "y1", "G_y1",
	      "y2", "G_y2");
    stmt->add("V",
	      "w", "G_w",
	      "x0", "U",
	      "x1", "tU",
	      "y0", "M0",
	      "y1", "M1",
	      "y2", "M2");

    scalarArgs = ([
	"w" : key->w(),
	"wprime" : key->wprime(),
	"x0" : key->x0(),
	"x1" : key->x1(),
	"y0" : key->y()[0],
	"y1" : key->y()[1],
	"y2" : key->y()[2]
    ]);
    params = PARAMS->credentialParams();
    pointArgs = ([
	"C_W" : key->C_W(),
	"G_w" : params[CRED_G_w],
	"G_wprime" : params[CRED_G_wprime],
	"G_V-I" : params[CRED_G_V] - key->I(),
	"G_x0" : params[CRED_G_x0],
	"G_x1" : params[CRED_G_x1],
	"G_y0" : params[CRED_G_y + 0],
	"G_y1" : params[CRED_G_y + 1],
	"G_y2" : params[CRED_G_y + 2],
	"V" : V,
	"U" : U,
	"tU" : U * t,
	"M0" : M[0],
	"M1" : M[1],
	"M2" : M[2]
    ]);

    proof = stmt->prove(scalarArgs, pointArgs, "", sho->squeeze(32));
}

/*
 * export as a blob
 */
string transport()
{
    return "\0\0\1\0\0\0\0\0\0" + proof + t->bytes() + U->bytes() + V->bytes();
}


Scalar t()		{ return t; }
RistrettoPoint U()	{ return U; }
RistrettoPoint V()	{ return V; }
string proof()		{ return proof; }
