#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct {
    int connfd;
    struct sockaddr_in addr;
} client_t;

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int r) {
    wasSigHup = 1;
}

int createServer(int port) {
    int serverSocket;
    struct sockaddr_in serverAddress;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(-1);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        perror("Socket bind failed");
        exit(-1);
    }

    if (listen(serverSocket, 3) != 0) {
        perror("Listen failed");
        exit(-1);
    }

    return serverSocket;
}

int main() {
    int serverSocket = createServer(8080);
    puts("Server is listening...\n");

    client_t clients[3];
    int activeClients = 0;
    int maxFd, i;
    char buffer[1024] = {0};

    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    while (1) {
        if (wasSigHup) {
            wasSigHup = 0;
            puts("Clients:");
            for (int i = 0; i < activeClients; i++) {
                printf("[%s:%d] ", inet_ntoa(clients[i].addr.sin_addr), htons(clients[i].addr.sin_port));
            }
            puts("\n");
        }
        
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(serverSocket, &fds);
        maxFd = serverSocket;

        for (int i = 0; i < activeClients; i++) {
            FD_SET(clients[i].connfd, &fds);
            if (clients[i].connfd > maxFd) {
                maxFd = clients[i].connfd;
            }
        }


        if (pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask) == -1 ) {
            if (errno != EINTR) {
                perror("Failed pselect()");
                return -1;
            }
        }

        if (FD_ISSET(serverSocket, &fds) && activeClients < 3) {
            client_t *client = &clients[activeClients];
            int len = sizeof(client->addr);
            int connfd = accept(serverSocket, (struct sockaddr*)&client->addr, &len);
            if (connfd >= 0) {
                client->connfd = connfd;
                printf("[%s:%d] Connected!\n", inet_ntoa(client->addr.sin_addr), htons(client->addr.sin_port));
                activeClients++;
            } else {
                perror("Connection error");
            }
        }

        for (i = 0; i < activeClients; i++) {
            client_t *client = &clients[i];
            if (FD_ISSET(client->connfd, &fds)) {
                int readLen = read(client->connfd, buffer, sizeof(buffer) - 1);
                if (readLen > 0) {
                    buffer[readLen - 1] = 0;
                    printf("[%s:%d] %s\n", inet_ntoa(client->addr.sin_addr), htons(client->addr.sin_port), buffer);
                } else {
                    close(client->connfd);
                    printf("[%s:%d]", inet_ntoa(client->addr.sin_addr), htons(client->addr.sin_port));
                    puts(" Connection closed\n");
                    clients[i] = clients[activeClients - 1];
                    activeClients--;
                    i--;
                }
            }
        }
    }

    return 0;
}
