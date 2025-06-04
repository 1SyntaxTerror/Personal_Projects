#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <poll.h>

#define SERVER_PORT 9091
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define NICKNAME_SIZE 32
#define MAX_CREDENTIALS 10

struct client_info {
    int fd;
    char nickname[NICKNAME_SIZE];
};

struct credential {
    char nickname[NICKNAME_SIZE];
    char password[NICKNAME_SIZE];
};

struct credential valid_users[MAX_CREDENTIALS] = {
    {"user1", "pass1"},
    {"user2", "pass2"},
    // Adaugă câți vrei
};
int valid_users_count = 2;

int check_credentials(const char *nickname, const char *password) {
    for (int i = 0; i < valid_users_count; i++) {
        if (strcmp(nickname, valid_users[i].nickname) == 0 &&
            strcmp(password, valid_users[i].password) == 0) {
            return 1; // valid
        }
    }
    return 0; // invalid
}

int main(void) {
    int server_fd;
    struct sockaddr_in server_address;

    struct client_info clients[MAX_CLIENTS];
    int client_count = 0;

    struct pollfd fds[MAX_CLIENTS + 1]; // +1 pentru server_fd

    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int option = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server pornit pe port %d\n", SERVER_PORT);

    // Setup poll fds
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1) {
        int poll_count = poll(fds, client_count + 1, -1);
        if (poll_count < 0) {
            perror("poll");
            break;
        }

        // Verific daca am conexiune noua
        if (fds[0].revents & POLLIN) {
            int new_fd = accept(server_fd, NULL, NULL);
            if (new_fd < 0) {
                perror("accept");
            } else if (client_count >= MAX_CLIENTS) {
                printf("Prea multi clienti conectati\n");
                close(new_fd);
            } else {
                // Primesc nickname:parola la conectare
                int bytes = recv(new_fd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes <= 0) {
                    close(new_fd);
                } else {
                    buffer[bytes] = '\0';
                    // Parse nickname and password
                    char *sep = strchr(buffer, ':');
                    if (!sep) {
                        char *msg = "Eroare: Format invalid (nickname:parola)\n";
                        send(new_fd, msg, strlen(msg), 0);
                        close(new_fd);
                        continue;
                    }
                    *sep = '\0';
                    char *nickname = buffer;
                    char *password = sep + 1;

                    // Verific daca numele este deja folosit
                    int name_taken = 0;
                    for (int i = 0; i < client_count; i++) {
                        if (strcmp(nickname, clients[i].nickname) == 0) {
                            name_taken = 1;
                            break;
                        }
                    }
                    if (name_taken) {
                        char *msg = "Eroare: Numele este deja folosit.\n";
                        send(new_fd, msg, strlen(msg), 0);
                        close(new_fd);
                        continue;
                    }

                    // Verific credentialele
                    if (!check_credentials(nickname, password)) {
                        char *msg = "Eroare: Date invalide.\n";
                        send(new_fd, msg, strlen(msg), 0);
                        close(new_fd);
                        continue;
                    }

                    // Adaug client valid
                    clients[client_count].fd = new_fd;
                    strncpy(clients[client_count].nickname, nickname, NICKNAME_SIZE);
                    clients[client_count].nickname[NICKNAME_SIZE - 1] = '\0';

                    fds[client_count + 1].fd = new_fd;
                    fds[client_count + 1].events = POLLIN;

                    client_count++;

                    printf("Client nou autentificat: %s (fd=%d)\n", nickname, new_fd);

                    // Trimit confirmare clientului
                    char *welcome = "Autentificare reusita\n";
                    send(new_fd, welcome, strlen(welcome), 0);
                }
            }
        }

        // Verific mesajele clientilor
        for (int i = 1; i <= client_count; i++) {
            if (fds[i].revents & POLLIN) {
                int fd = fds[i].fd;
                int bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes <= 0) {
                    // Client a inchis conexiunea
                    printf("Client %s a deconectat\n", clients[i-1].nickname);
                    close(fd);

                    // Scot clientul din lista
                    for (int j = i; j < client_count; j++) {
                        clients[j-1] = clients[j];
                        fds[j] = fds[j+1];
                    }
                    client_count--;
                    i--;
                } else {
                    buffer[bytes] = '\0';

                    if (strcmp(buffer, "exit") == 0) {
                        printf("Client %s a cerut inchiderea conexiunii\n", clients[i-1].nickname);
                        close(fd);

                        for (int j = i; j < client_count; j++) {
                            clients[j-1] = clients[j];
                            fds[j] = fds[j+1];
                        }
                        client_count--;
                        i--;
                    } else {
                        // Construiesc mesaj cu nickname
                        char message[BUFFER_SIZE + NICKNAME_SIZE + 3];
                        snprintf(message, sizeof(message), "%s: %s", clients[i-1].nickname, buffer);

                        printf("%s\n", message);

                        // Trimit mesajul catre toti ceilalti clienti
                        for (int j = 0; j < client_count; j++) {
                            if (clients[j].fd != fd) {
                                send(clients[j].fd, message, strlen(message), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
