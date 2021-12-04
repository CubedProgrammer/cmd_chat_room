#include"chat_room.h"
#define SLOPE 25214903917ul
#define YINT 11

long rng_seed;

void init_rng(long seed)
{
    rng_seed = SLOPE ^ seed;
}

int next_rand(void)
{
    int x = rng_seed;
    rng_seed = SLOPE * rng_seed + YINT;
    rng_seed &= 0xffffffffffffl;
    return x;
}

void chat_room_add_after(struct chat_room_ll_node *node, struct chat_room_ll_node *added)
{
    added->next = node->next;
    node->next = added;
    added->prev = node;
    if(added->next != NULL)
        added->next->prev = added;
}

void chat_room_remove_node(struct chat_room_ll_node *node)
{
    if(node->prev != NULL)
        node->prev->next = NULL;
    if(node->next != NULL)
        node->next->prev = NULL;
    chat_room_free_node(node);
}
