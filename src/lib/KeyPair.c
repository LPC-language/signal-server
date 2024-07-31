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


private Scalar w;
private Scalar wprime;
private RistrettoPoint W;
private Scalar x0;
private Scalar x1;
private Scalar *y;
private RistrettoPoint C_W;
private RistrettoPoint I;

static void create(Sho sho, int size, varargs int nattr)
{
    mixed *params;
    int i;

    if (!nattr) {
	nattr = size;
    }

    params = PARAMS->credentialParams();
    w = sho->getScalar();
    W = params[CRED_G_w] * w;
    wprime = sho->getScalar();
    x0 = sho->getScalar();
    x1 = sho->getScalar();

    y = allocate(size);
    for (i = 0; i < size; i++) {
	y[i] = sho->getScalar();
    }
    C_W = params[CRED_G_w] * w + params[CRED_G_wprime] * wprime;
    I = params[CRED_G_V] - params[CRED_G_x0] * x0 - params[CRED_G_x1] * x1;
    for (i = 0; i < nattr; i++) {
	I -= params[CRED_G_y + i] * y[i];
    }
}

mixed *prepare(Sho sho, RistrettoPoint *M)
{
    Scalar t;
    RistrettoPoint U, V;
    int size, i;

    t = sho->getScalar();
    U = sho->getPoint();
    V = W + U * (x0 + x1 * t);
    for (i = 0, size = sizeof(M); i < size; i++) {
	V += M[i] * y[i];
    }

    return ({ t, U, V });
}


Scalar w()		{ return w; }
Scalar wprime()		{ return wprime; }
RistrettoPoint W()	{ return W; }
Scalar x0()		{ return x0; }
Scalar x1()		{ return x1; }
Scalar *y()		{ return y[..]; }
RistrettoPoint C_W()	{ return C_W; }
RistrettoPoint I()	{ return I; }
