#include <iostream>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

using namespace std;

#define PORT 8080
#define MAX_CLIENTS 10

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int r) {
    wasSigHup = 1;
}

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    fd_set readfds;
    int max_sd, sd, activity;
    int clients[MAX_CLIENTS] = {0};
    int max_clients = MAX_CLIENTS;
    int client_socket, i;

    // Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Регистрация обработчика сигнала
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    // Блокирование сигнала
    sigset_t blockedMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, NULL);

    // Установка опций сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Привязка сокета к адресу и порту
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Прослушивание сокета
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Основной цикл приложения
    while (1) {
        // Подготовка списка файловых дескрипторов
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;
        for (i = 0; i < max_clients; i++) {
            sd = clients[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Вызов функции pselect()
        activity = pselect(max_sd + 1, &readfds, NULL, NULL, NULL, &blockedMask);

        if (activity < 0 && errno != EINTR) {
            perror("pselect error");
        }

        // Обработка произошедшего события
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            cout << "New connection: " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << endl;

            // Оставляем только одно соединение
            for (i = 0; i < max_clients; i++) {
                if (clients[i] == 0) {
                    clients[i] = new_socket;
                    break;
                }
                else {
                    close(new_socket);
                }
            }
        }

        for (i = 0; i < max_clients; i++) {
            client_socket = clients[i];
            if (FD_ISSET(client_socket, &readfds)) {
                if ((valread = read(client_socket, buffer, 1024)) == 0) {
                    getpeername(client_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    cout << "Host disconnected: " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << endl;
                    close(client_socket);
                    clients[i] = 0;
                }
                else {
                    cout << "Data received: " << valread << endl;
                }
            }
        }

        if (wasSigHup) {
            cout << "SIGHUP received" << endl;
            wasSigHup = 0;
        }
    }

    return 0;
}
