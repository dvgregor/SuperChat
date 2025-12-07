#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <stdbool.h>

void talkwithclient(int sock);

bool g_nonblock = 1;

int main(int argc, char **argv)
{
    int sockfd, newsockfd, portno;
    socklen_t cli_addr_len;
    struct sockaddr_in srv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
    }

    if (g_nonblock)
    {
        int flags = fcntl(sockfd, F_GETFL, 0);

        if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) != -1)
        {
            fprintf(stderr, "Listening socket got O_NONBLOCK flag\n");
        }
        else
        {
            fprintf(stderr, "ERROR Listening socket failed to got O_NONBLOCK flag\n");
            exit(1);
        }
    }

    bzero((char *) &srv_addr, sizeof(srv_addr));
    portno = atoi(argv[1]);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0)
    {
        fprintf(stderr, "ERROR on binding\n");
        exit(1);
    }

    listen(sockfd, 5);
    cli_addr_len = sizeof(cli_addr);

    while (1) 
    {
        pid_t pid;

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_addr_len);

        if (newsockfd >= 0)
        {
            fprintf(stderr, "Connection accepted, newsockfd %d\n", newsockfd);
        }
        else
        {
            int err = errno;

            if (g_nonblock && (err == EAGAIN || err == EWOULDBLOCK))
            {
                struct timespec ts;

                fprintf(stderr, "Wait a little and invoke accept again\n");

                ts.tv_sec = 2;
                ts.tv_nsec = 0;
                nanosleep(&ts, &ts);
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR on accept, newsockfd %d, errno %d\n", newsockfd, err);
                exit(1);
            }
        }

        pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "ERROR on binding\n");
            exit(1);
        }

        if (pid == 0)
        {
            close(sockfd);
            talkwithclient(newsockfd);
            exit(0);
        }
        else 
            close(newsockfd);
    }

    close(sockfd);
    return 0; /* we'll never get here */
}

void talkwithclient(int sock)
{
    int n;
    char buffer[256];
        
    bzero(buffer, sizeof(buffer));
    n = read(sock, buffer, sizeof(buffer));

    if (n < 0)
    {
        int err = errno;
        fprintf(stderr, "ERROR reading from socket, errno %d\n", err);
    }

    printf("Message: %s\n", buffer);

    n = write(sock, "I've got your message", sizeof("I've got your message"));

    if (n < 0)
    {
        int err = errno;
        fprintf(stderr, "ERROR writing to socket, errno %d\n", err);
    }
}
