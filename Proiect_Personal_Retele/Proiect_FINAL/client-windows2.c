#define _WIN32_WINNT 0x0601
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

SOCKET socket_fd;
int running = 1;

void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

void *recv_thread_func(void *arg) {
    char buffer[BUFFER_SIZE];
    while (running) {
        int bytes_received = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("\nConexiunea cu serverul a fost închisă sau a apărut o eroare.\n");
            running = 0;
            break;
        }
        buffer[bytes_received] = '\0';
        printf("\n%s\n> ", buffer);
        fflush(stdout);
    }
    return NULL;
}

void *send_thread_func(void *arg) {
    char buffer[BUFFER_SIZE];
    while (running) {
        printf("> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            running = 0;
            break;
        }

        trim_newline(buffer);

        if (strcmp(buffer, "exit") == 0) {
            running = 0;
            shutdown(socket_fd, SD_BOTH);
            break;
        }

        send(socket_fd, buffer, strlen(buffer), 0);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Folosește: %s <IP_SERVER/DOMENIU> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_host = argv[1];
    const char *server_port = argv[2];

    char nickname[BUFFER_SIZE];
    char password[BUFFER_SIZE];

    printf("Introdu numele tău: ");
    if (fgets(nickname, BUFFER_SIZE, stdin) == NULL) {
        fprintf(stderr, "Eroare la citirea numelui\n");
        exit(EXIT_FAILURE);
    }
    trim_newline(nickname);

    printf("Introdu parola ta: ");
    if (fgets(password, BUFFER_SIZE, stdin) == NULL) {
        fprintf(stderr, "Eroare la citirea parolei\n");
        exit(EXIT_FAILURE);
    }
    trim_newline(password);

    // Inițializare WinSock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Eroare la WSAStartup\n");
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints, *res, *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(server_host, server_port, &hints, &res)) != 0) {
        fprintf(stderr, "Eroare la getaddrinfo: %s\n", gai_strerrorA(status));
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_fd == INVALID_SOCKET) continue;

        if (connect(socket_fd, p->ai_addr, (int)p->ai_addrlen) == 0) {
            break;
        }

        closesocket(socket_fd);
    }

    if (p == NULL) {
        fprintf(stderr, "Eroare: nu s-a putut stabili conexiunea\n");
        freeaddrinfo(res);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    // Trimitem nickname:parola
    char auth_data[BUFFER_SIZE * 2];
    snprintf(auth_data, sizeof(auth_data), "%s:%s", nickname, password);
    send(socket_fd, auth_data, strlen(auth_data), 0);

    // Așteptăm confirmarea
    char response[BUFFER_SIZE];
    int bytes = recv(socket_fd, response, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        fprintf(stderr, "Conexiune închisă de server\n");
        closesocket(socket_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    response[bytes] = '\0';

    if (strstr(response, "reusita") == NULL) {
        fprintf(stderr, "Autentificare eșuată.\n");
        closesocket(socket_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("%s", response);
    printf("Conectat la server %s:%s ca '%s'\n", server_host, server_port, nickname);

    pthread_t recv_thread, send_thread;
    pthread_create(&recv_thread, NULL, recv_thread_func, NULL);
    pthread_create(&send_thread, NULL, send_thread_func, NULL);

    pthread_join(send_thread, NULL);
    running = 0;
    shutdown(socket_fd, SD_BOTH);
    pthread_join(recv_thread, NULL);

    closesocket(socket_fd);
    WSACleanup();

    printf("Client închis.\n");
    return 0;
}
