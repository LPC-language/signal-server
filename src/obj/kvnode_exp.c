# include <KVstore.h>

inherit KVnode;


/*
 * path of this node
 */
static string nodePath()
{
    return "/usr/MsgServer/obj/kvnode_exp";
}

/*
 * value with expiration
 */
static mixed keyValue(string key, mixed value)
{
    return (value[0] - time() > 0) ? value : nil;
}
