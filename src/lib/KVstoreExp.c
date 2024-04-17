# include <KVstore.h>

inherit KVstore;


private int duration;	/* K/V duration */

/*
 * KVstore with expiration
 */
static void create(int maxSize, int duration)
{
    ::create(maxSize, nil, "/usr/MsgServer/obj/kvnode_exp");
    ::duration = duration;
}

/*
 * get K/V
 */
mixed get(string key)
{
    mixed value;

    value = ::get(key);
    return (value) ? value[1] : nil;
}

/*
 * get duration
 */
int getDuration(string key)
{
    mixed value;

    value = ::get(key);
    return (value) ? value[0] - time() : 0;
}

/*
 * set K/V
 */
void set(string key, mixed value)
{
    ::set(key, ({ time() + duration, value }));
}

/*
 * add K/V
 */
void add(string key, mixed value)
{
    ::add(key, ({ time() + duration, value }));
}

/*
 * change K/V
 */
void change(string key, mixed value)
{
    ::change(key, ({ time() + duration, value }));
}
