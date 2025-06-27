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
 * initialize AuthCredentialWithPniResponse
 */
static void create(string uuid, string pni, int redemptionTime, string random)
{
    RistrettoPoint *params, *M;
    Sho sho;
    KeyPair key;
    Statement stmt;
    mapping scalarArgs, pointArgs;

    params = PARAMS->credentialParams();
    M = uuid::points(uuid) + uuid::points(pni) + ({
	params[CRED_G_m5] * timeScalar(redemptionTime)
    });

    sho = PARAMS->authCredentialWithPniSho();
    sho->absorb(random);
    sho->ratchet();

    key = CREDENTIALS_SERVER->authCredentialWithPniKey();
    ({ t, U, V }) = key->prepare(sho, M);

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
    stmt->add("V",
	      "w", "G_w",
	      "x0", "U",
	      "x1", "tU",
	      "y1", "M1",
	      "y2", "M2",
	      "y3", "M3",
	      "y4", "M4",
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
	"y5" : key->y()[4]
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
	"V" : V,
	"U" : U,
	"tU" : U * t,
	"M1" : M[0],
	"M2" : M[1],
	"M3" : M[2],
	"M4" : M[3],
	"M5" : M[4]
    ]);

    proof = stmt->prove(scalarArgs, pointArgs, "", sho->squeeze(32));
}

/*
 * export as a blob
 */
string transport()
{
    return "\0" + t->bytes() + U->bytes() + V->bytes() +
	   "\x40\1\0\0\0\0\0\0" + proof;
}


Scalar t()		{ return t; }
RistrettoPoint U()	{ return U; }
RistrettoPoint V()	{ return V; }
string proof()		{ return proof; }
