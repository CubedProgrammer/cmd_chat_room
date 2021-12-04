#include<arpa/inet.h>
#include<pthread.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<time.h>
#include<unistd.h>
#include"chat_room.h"
#define PORT 8080
#define RDCHR(fd, ch) read(fd, &ch, sizeof(ch))
#define PUTOBJ(fd, obj) write(fd, &obj, sizeof(obj))
struct chat_room_ll_node *chr_head;
void *handle_client(void *arg)
{
    struct chat_room_ll_node **np = arg;
    struct chat_room *rmp = &(*np)->rm;
    int cli = *(int*)(np + 1);
    int *otherp = cli == rmp->ffd ? &rmp->sfd : &rmp->ffd;
    int other;
    const char *name = cli == rmp->ffd ? rmp->fname : rmp->sname;
    short namlen = strlen(name);
    namlen = htons(namlen);
    char msg[2601];
    char msgt = 29;
    short msglen;
    while(msgt == 29)
    {
        RDCHR(cli, msgt);
        if(msgt == 29)
        {
            other = *otherp;
            RDCHR(cli, msglen);
            msglen = ntohs(msglen);
            msglen = read(cli, msg, msglen);
            PUTOBJ(cli, msgt);
            PUTOBJ(cli, namlen);
            write(cli, name, ntohs(namlen));
            msglen = htons(msglen);
            PUTOBJ(cli, msglen);
            write(cli, msg, ntohs(msglen));
            PUTOBJ(other, msgt);
            PUTOBJ(other, namlen);
            write(other, name, ntohs(namlen));
            PUTOBJ(other, msglen);
            write(other, msg, ntohs(msglen));
        }
        else if(msgt == 31)
        {
            if(rmp->sfd > 0)
            {
                PUTOBJ(rmp->sfd, msgt);
                close(rmp->sfd);
            }
            rmp->sfd = -1;
            if(cli == rmp->ffd)
            {
                PUTOBJ(rmp->ffd, msgt);
                close(rmp->ffd);
                rmp->ffd = -1;
                chat_room_remove_node(*np);
            }
        }
    }
    return NULL;
}
void *accept_clients(void *arg)
{
    char usrnm[31];
    pthread_t **thsp = arg;
    pthread_t *ths = *thsp;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in saddr;
    socklen_t sa_len = sizeof(saddr);
    socklen_t *sa_lenp = &sa_len;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(PORT);
    saddr.sin_family = AF_INET;
    bind(sock, (struct sockaddr *)&saddr, sa_len);
    listen(sock, 3);
    int thcnt = 1, capa = 2;
    int cli;
    char cmd;
    int namlen, rnum;
    struct chat_room_ll_node *tmpnd, **th_arg;
    for(;;)
    {
        cli = accept(sock, (struct sockaddr *)&saddr, sa_lenp);
        RDCHR(cli, cmd);
        namlen = read(cli, usrnm, cmd);
        usrnm[namlen] == '\0';
        RDCHR(cli, cmd);
        if(cmd == 17)
        {
            RDCHR(cli, rnum);
            rnum = ntohl(rnum);
            cmd = 23;
            for(struct chat_room_ll_node *search = tmpnd; search != NULL; search = search->next)
            {
                if(search->rm.rnum == rnum)
                {
                    if(search->rm.sfd == -1)
                    {
                        search->rm.sname = malloc(namlen + 1);
                        strcpy(search->rm.sname, usrnm);
                        search->rm.sfd = cli;
                        cmd = namlen;
                        printf("Room %x has been filled.\n", search->rm.rnum);
                        cmd = 17;
                    }
                    else
                        cmd = 19;
                }
            }
            PUTOBJ(cli, cmd);
        }
        else if(cmd == 13)
        {
            tmpnd = mk_chat_room();
            tmpnd->rm.fname = malloc(namlen + 1);
            strcpy(tmpnd->rm.fname, usrnm);
            tmpnd->rm.ffd = cli;
            if(chr_head == NULL)
                chr_head = tmpnd;
            else
                chat_room_add_after(chr_head, tmpnd);
            rnum = htonl(tmpnd->rm.rnum);
            PUTOBJ(cli, rnum);
            printf("Room %x has been created.\n", tmpnd->rm.rnum);
        }
        if(thcnt == capa)
        {
            capa += capa >> 1;
            *thsp = realloc(*thsp, capa);
            ths = *thsp;
        }
        th_arg = malloc(sizeof(int) + sizeof(*th_arg));
        *th_arg = tmpnd;
        *(int*)(th_arg + 1) = cli;
        pthread_create(ths + thcnt, NULL, handle_client, th_arg);
        ++thcnt;
    }
    return NULL;
}
int main(int argl, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGPIPE, SIG_IGN);
    struct timespec ctm;
    clock_gettime(CLOCK_REALTIME, &ctm);
    init_rng(ctm.tv_sec ^ ctm.tv_nsec);
    pthread_t acc_th;
    pthread_t *pths = malloc(2 * sizeof(pthread_t));
    pthread_t **acc_arg = malloc(sizeof(pthread_t *));
    *acc_arg = pths;
    pthread_create(&acc_th, NULL, accept_clients, acc_arg);
    fgetc(stdin);
    free(*acc_arg);
    free(acc_arg);
    return 0;
}
