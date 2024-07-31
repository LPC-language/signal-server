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

# include "Sho.h"
# include "zkp.h"


private string *equations;	/* ZKP equations */
private mapping scalarMap;	/* string : ScalarIndex */
private string *scalars;	/* scalar names */
private mapping pointMap;	/* string : PointIndex */
private string *points;		/* point names */

/*
 * initialize statement
 */
static void create()
{
    equations = ({ });
    scalarMap = ([ ]);
    scalars = ({ });
    pointMap = ([ "G" : 0 ]);
    points = ({ "G" });
}

/*
 * add a scalar
 */
private int addScalar(string name)
{
    mixed value;
    int index;

    value = scalarMap[name];
    if (value == nil) {
	index = sizeof(scalars);
	if (index > 255) {
	    error("Too many scalars");
	}

	scalarMap[name] = index;
	scalars += ({ name });

	return index;
    } else {
	return value;
    }
}

/*
 * add a point
 */
private int addPoint(string name)
{
    mixed value;
    int index;

    value = pointMap[name];
    if (value == nil) {
	index = sizeof(points);
	if (index > 255) {
	    error("Too many points");
	}

	pointMap[name] = index;
	points += ({ name });

	return index;
    } else {
	return value;
    }
}

/*
 * add an equation
 */
void add(string lhs, string rhs...)
{
    int size, i;
    string lhsSize, term, *terms;

    size = sizeof(rhs);
    if (size == 0 || (size & 1) || size > 255 * 2) {
	error("Bad arguments");
    }
    if (sizeof(equations) >= 255) {
	error("Too many equations");
    }

    size /= 2;
    lhsSize = "..";
    lhsSize[0] = addPoint(lhs);
    lhsSize[1] = size;
    terms = allocate(size);
    term = "..";
    for (i = 0; i < size; i++) {
	term[0] = addScalar(rhs[2 * i]);
	term[1] = addPoint(rhs[2 * i + 1]);
	terms[i] = term;
    }
    equations += ({ lhsSize + implode(terms, "") });
}

/*
 * a description of the system of equations
 */
string description()
{
    string c;

    c = ".";
    c[0] = sizeof(equations);
    return c + implode(equations, "");
}

/*
 * scalars in ordered list
 */
private Scalar *getScalars(mapping scalarArgs)
{
    int size, i;
    Scalar *args, scalar;

    size = map_sizeof(scalarArgs);
    if (size != sizeof(scalars)) {
	error("Bad scalar arguments");
    }
    args = allocate(size);
    for (i = 0; i < size; i++) {
	scalar = scalarArgs[scalars[i]];
	if (!scalar) {
	    error("Missing scalar argument");
	}
	args[i] = scalar;
    }

    return args;
}

/*
 * points in ordered list
 */
private RistrettoPoint *getPoints(mapping pointArgs)
{
    int size, i;
    RistrettoPoint *args, point;

    pointArgs["G"] = new RistrettoPoint(ristretto255_basepoint());

    size = map_sizeof(pointArgs);
    if (size != sizeof(points)) {
	error("Bad point arguments");
    }
    args = allocate(size);
    for (i = 0; i < size; i++) {
	point = pointArgs[points[i]];
	if (!point) {
	    error("Missing point argument");
	}
	args[i] = point;
    }

    return args;
}

/*
 * See:
 * "A Graduate Course in Applied Cryptography," Boneh and Shoup, section 19.5.3
 */
private RistrettoPoint *homomorphism(Scalar *g1, RistrettoPoint *points)
{
    int size, i, esize, j;
    RistrettoPoint *g2, point;
    string equation;

    size = sizeof(equations);
    g2 = allocate(size);
    for (i = 0; i < size; i++) {
	equation = equations[i];
	point = points[equation[3]] * g1[equation[2]];
	for (j = 1, esize = equation[1]; j < esize; j++) {
	    point += points[equation[2 * j + 3]] * g1[equation[2 * j + 2]];
	}
	g2[i] = point;
    }

    return g2;
}

/*
 * generate a proof
 */
string prove(mapping scalarArgs, mapping pointArgs, string message,
	     string randomBytes)
{
    Scalar *g1;
    RistrettoPoint *points, *commitment;
    Sho sho, sho2;
    int size, i;
    string data, proof;
    Scalar *nonce, challenge, *response;

    g1 = getScalars(scalarArgs);
    points = getPoints(pointArgs);

    sho = new ShoHmacSha256("POKSHO_Ristretto_SHOHMACSHA256");
    sho->absorb(description());
    for (i = 0, size = sizeof(points); i < size; i++) {
	sho->absorb(points[i]->bytes());
    }
    sho->ratchet();

    /* produce nonce */
    sho2 = sho->clone();
    sho2->absorb(randomBytes);
    for (i = 0, size = sizeof(g1); i < size; i++) {
	sho2->absorb(g1[i]->bytes());
    }
    sho2->ratchet();
    sho2->absorb(message);
    sho2->ratchet();
    data = sho2->squeeze(size * 64);
    nonce = allocate(size);
    for (i = 0; i < size; i++) {
	nonce[i] = new Scalar(data[i * 64 .. i * 64 + 63]);
    }

    /* commitment & challenge */
    commitment = homomorphism(nonce, points);
    for (i = 0, size = sizeof(commitment); i < size; i++) {
	sho->absorb(commitment[i]->bytes());
    }
    sho->absorb(message);
    sho->ratchet();
    challenge = new Scalar(sho->squeeze(64));

    /* proof */
    size = sizeof(g1);
    response = allocate(size);
    for (i = 0; i < size; i++) {
	response[i] = g1[i] * challenge + nonce[i];
    }
    return new Proof(challenge, response)->transport();
}

/*
 * verify a proof
 */
int verify(string proofBytes, mapping pointArgs, string message)
{
    Proof proof;
    RistrettoPoint *points, point, *commitment;
    Sho sho;
    int i, size;
    Scalar challenge;

    proof = new RemoteProof(proofBytes);
    if (sizeof(proof->response()) != sizeof(scalars)) {
	return FALSE;
    }
    points = getPoints(pointArgs);

    sho = new ShoHmacSha256("POKSHO_Ristretto_SHOHMACSHA256");
    sho->absorb(description());
    for (i = 0, size = sizeof(points); i < size; i++) {
	sho->absorb(points[i]->bytes());
    }
    sho->ratchet();

    /* reproduce commitment & challenge */
    challenge = proof->challenge();
    commitment = homomorphism(proof->response(), points);
    for (i = 0, size = sizeof(commitment); i < size; i++) {
	point = commitment[i] - points[equations[i][0]] * challenge;
	sho->absorb(point->bytes());
    }
    sho->absorb(message);
    sho->ratchet();

    return challenge->equals(new Scalar(sho->squeeze(64)));
}
