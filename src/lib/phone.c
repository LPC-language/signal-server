# include <limits.h>

inherit asn "/lib/util/asn";


# define LIMIT		"\1\0\0\0\0\0\0\0\0"
# define TEN_MILLION	"\x00\x98\x96\x80"

/*
 * convert "+1551234567" to 8-byte bigendian number
 */
static string phoneToNum(string str)
{
    string num;
# if INT_MIN == 0x80000000
    string multiplier;

    num = "\0";
    multiplier = "\1";
    for (str = str[1 ..]; strlen(str) > 7; str = str[.. strlen(str) - 8]) {
	num = asn_add(num,
		      asn_mult(asn::encode((int) str[strlen(str) - 7 ..]),
			       multiplier, LIMIT),
		      LIMIT);
	multiplier = asn_mult(multiplier, TEN_MILLION, LIMIT);
    }
    num = asn_add(num, asn_mult(asn::encode((int) str), multiplier, LIMIT),
		  LIMIT);

# else
    num = asn::encode((int) str[1 ..]);
# endif

    num = asn::extend(num, 8);
    return num;
}

/*
 * convert 8-byte bigendian to "+15551234567" phone number
 */
static string numToPhone(string str)
{
# if INT_MIN == 0x80000000
    string num, d;
    int n;

    for (num = ""; asn_cmp(str, TEN_MILLION) >= 0;
	 str = asn_div(str, TEN_MILLION, str)) {
	d = asn_mod(str, TEN_MILLION);
	n = 10000000 + (d[0] << 16) + (d[1] << 8) + d[2];
	num = ((string) n)[1 ..] + num;
    }

    for (n = 0; strlen(str) != 0; str = str[1 ..]) {
	n = (n << 8) + str[0];
    }
    return "+" + (string) n + num;
# else
    return "+" + asn::decode(str);
# endif
}
