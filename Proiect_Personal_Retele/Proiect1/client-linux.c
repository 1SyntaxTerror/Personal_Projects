#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

int socket_fd;
int running = 1;

void *recv_thread_func(void *arg)
{
    char buffer[BUFFER_SIZE];
    while (running)
    {
        int bytes_received = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0)
        {
            printf("\nConexiunea cu serverul a fost inchisă sau eroare.\n");
            running = 0;
            break;
        }
        buffer[bytes_received] = '\0';
        printf("\n%s\n> ", buffer);
        fflush(stdout);
    }
    return NULL;
}

void *send_thread_func(void *arg)
{
    char buffer[BUFFER_SIZE];
    while (running)
    {
        printf("> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
        {
            running = 0;
            break;
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0)
        {
            running = 0;
            shutdown(socket_fd, SHUT_RDWR);
            break;
        }

        send(socket_fd, buffer, strlen(buffer), 0);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Folosește: %s <IP_SERVER> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    char nickname[BUFFER_SIZE];
    printf("Introdu numele tau: ");
    if (fgets(nickname, BUFFER_SIZE, stdin) == NULL)
    {
        fprintf(stderr, "Eroare la citirea numelui\n");
        exit(EXIT_FAILURE);
    }
    size_t len = strlen(nickname);
    if (len > 0 && nickname[len - 1] == '\n')
        nickname[len - 1] = '\0';

    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    //     fprintf(stderr, "Adresă IP invalidă.\n");
    //     exit(EXIT_FAILURE);
    // }
    // Varianta fara a transforma adresa serveo.net in adresa cu numere

    struct hostent *he = gethostbyname(server_ip);
    if (he == NULL)
    {
        fprintf(stderr, "Nu pot rezolva hostname-ul %s\n", server_ip);
        exit(EXIT_FAILURE);
    }
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Trimit numele la server dupa conectare
    send(socket_fd, nickname, strlen(nickname), 0);

    printf("Conectat la server %s:%d ca '%s'\n", server_ip, server_port, nickname);

    pthread_t recv_thread, send_thread;
    pthread_create(&recv_thread, NULL, recv_thread_func, NULL);
    pthread_create(&send_thread, NULL, send_thread_func, NULL);

    pthread_join(send_thread, NULL);
    running = 0;
    shutdown(socket_fd, SHUT_RDWR);
    pthread_join(recv_thread, NULL);

    close(socket_fd);

    printf("Client închis.\n");
    return 0;
}
