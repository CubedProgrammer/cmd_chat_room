#include<stdlib.h>

extern long rng_seed;

struct chat_room
{
    int rnum;
    int ffd, sfd;
    char *fname, *sname;
};

struct chat_room_ll_node
{
    struct chat_room rm;
    struct chat_room_ll_node *next, *prev;
};

void init_rng(long seed);
int next_rand(void);
void chat_room_add_after(struct chat_room_ll_node *node, struct chat_room_ll_node *added);
void chat_room_remove_node(struct chat_room_ll_node *node);

static inline void chat_room_free_node(struct chat_room_ll_node *node)
{
    free(node->rm.fname);
    free(node->rm.sname);
    free(node);
}

static inline struct chat_room_ll_node *mk_chat_room(void)
{
    struct chat_room_ll_node *node = malloc(sizeof(struct chat_room_ll_node));
    node->rm.rnum = next_rand();
    node->rm.ffd = -1;
    node->rm.sfd = -1;
    node->rm.fname = NULL;
    node->rm.sname = NULL;
    node->prev = NULL;
    node->next = NULL;
    return node;
}
