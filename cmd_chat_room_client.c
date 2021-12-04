#include<arpa/inet.h>
#include<pthread.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<termios.h>
#include<time.h>
#include<unistd.h>
#include<cpcou_misc_utils.h>
#define PORT 8080
#define RDCHR(fd, ch) read(fd, &ch, sizeof(ch))
#define PUTOBJ(fd, obj) write(fd, &obj, sizeof(obj))
#define gch getchar()
char msg[2601];
void *chat_receive(void *arg)
{
    int sock = *(int *)arg;
    char msgt;
    short msglen, nmsglen;
    RDCHR(sock, msgt);
    while(msgt != 31)
    {
        if(msgt == 29)
        {
            RDCHR(sock, nmsglen);
            msglen = ntohs(nmsglen);
            msglen = read(sock, msg, msglen);
            msg[msglen] = '\0';
            printf("\033\1331m%s at %li:\033\1330m ", msg, time(NULL));
            RDCHR(sock, nmsglen);
            msglen = ntohs(nmsglen);
            msglen = read(sock, msg, msglen);
            msg[msglen] = '\0';
            puts(msg);
        }
        RDCHR(sock, msgt);
    }
    return NULL;
}
void chat(int sock)
{
    pthread_t pth;
    pthread_create(&pth, NULL, chat_receive, &sock);
    puts("Press Ctrl+B to start a message, type enter to send, press Ctrl+O to disconnect.");
    char cmd = gch, msgt;
    short msglen, nmsglen;
    while(cmd != 15)
    {
        switch(cmd)
        {
            case 2:
                fputs("Message: ", stdout);
                cpcou_get_password(msg, 2600, 0, 1);
                msglen = strlen(msg);
                if(msglen != 0)
                {
                    msgt = 29;
                    nmsglen = htons(msglen);
                    PUTOBJ(sock, msgt);
                    PUTOBJ(sock, nmsglen);
                    write(sock, msg, msglen);
                    fputc('\r', stdout);
                }
                else
                    fputs("\b\b\b\b\b\b\b\b\b        \b\b\b\b\b\b\b\b", stdout);
                break;
            default:
                fputc('\a', stdout);
        }
        cmd = gch;
    }
    cmd = 31;
    PUTOBJ(sock, cmd);
    pthread_join(pth, NULL);
}
int main(int argl, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGPIPE, SIG_IGN);
    const char *addrs = argl == 2 ? argv[1] : "127.0.0.1";
    char name[31];
    char rmbuf[9];
    struct sockaddr_in saddr;
    socklen_t saddr_len = sizeof(saddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    inet_pton(AF_INET, addrs, &saddr.sin_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~ECHO & ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    char st, namlen;
    unsigned room;
    int quit = 0;
    while(!quit)
    {
        fputs("Press Ctrl+O to quit or disconnect, type c to create a chat room, or j to join a chat room.", stdout);
        fputc('\r', stdout);
        st = gch;
        switch(st)
        {
            case'c':
            case'C':
                fputc('\n', stdout);
                fputs("Enter your name: ", stdout);
                cpcou_get_password(name, 30, 1, 1);
                puts(name);
                namlen = strlen(name);
                st = 13;
                connect(sock, (struct sockaddr *)&saddr, saddr_len);
                PUTOBJ(sock, namlen);
                write(sock, name, namlen);
                PUTOBJ(sock, st);
                RDCHR(sock, room);
                room = ntohl(room);
                printf("Room %08x has been created.\n", room);
                chat(sock);
                break;
            case'j':
            case'J':
                fputc('\n', stdout);
                fputs("Enter your name: ", stdout);
                cpcou_get_password(name, 30, 1, 1);
                puts(name);
                fputs("Enter the room you wish to join: ", stdout);
                cpcou_get_password(rmbuf, 8, 1, 0);
                fputc('\n', stdout);
                namlen = strlen(name);
                st = 17;
                room = 0;
                for(const char *it = rmbuf; *it != '\0'; ++it)
                {
                    room *= 16;
                    if(*it >= '0' && *it <= '9')
                        room += *it - '0';
                    else if(*it >= 'a' && *it <= 'f')
                        room += *it - 'a' + 10;
                    else if(*it >= 'A' && *it <= 'F')
                        room += *it - 'A' + 10;
                }
                connect(sock, (struct sockaddr *)&saddr, saddr_len);
                PUTOBJ(sock, namlen);
                write(sock, name, namlen);
                PUTOBJ(sock, st);
                room = htonl(room);
                PUTOBJ(sock, room);
                RDCHR(sock, st);
                if(st == 17)
                {
                    puts("\033\13332mSuccess.\033\1330m");
                    chat(sock);
                }
                else if(st == 19)
                    puts("\033\13331mAlready full.\033\1330m");
                else
                    puts("\033\13331mRoom not found.\033\1330m");
                break;
            case 15:
                quit = 1;
                fputc('\n', stdout);
            default:
                if(st > ' ')
                    fputc('\a', stdout);
        }
    }
    term.c_lflag |= ECHO | ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    return 0;
}
