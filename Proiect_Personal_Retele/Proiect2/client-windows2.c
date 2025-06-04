#define _WIN32_WINNT 0x0601 // Windows 7 sau mai nou
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_SIZE 1024

SOCKET socket_fd;
volatile int running = 1;

void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

DWORD WINAPI recv_thread_func(LPVOID arg)
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
    return 0;
}

DWORD WINAPI send_thread_func(LPVOID arg)
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

        trim_newline(buffer);

        if (strcmp(buffer, "exit") == 0)
        {
            running = 0;
            shutdown(socket_fd, SD_BOTH);
            break;
        }

        send(socket_fd, buffer, (int)strlen(buffer), 0);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Folosește: %s <IP_SERVER> <PORT>\n", argv[0]);
        return 1;
    }

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", iResult);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    char nickname[BUFFER_SIZE];
    char password[BUFFER_SIZE];

    // Citire nickname si eliminare newline
    printf("Introdu numele tau: ");
    if (fgets(nickname, BUFFER_SIZE, stdin) == NULL)
    {
        fprintf(stderr, "Eroare la citirea numelui\n");
        WSACleanup();
        return 1;
    }
    trim_newline(nickname);

    // Citire parola si eliminare newline
    printf("Introdu parola ta: ");
    if (fgets(password, BUFFER_SIZE, stdin) == NULL)
    {
        fprintf(stderr, "Eroare la citirea parolei\n");
        WSACleanup();
        return 1;
    }
    trim_newline(password);

    struct sockaddr_in server_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == INVALID_SOCKET)
    {
        fprintf(stderr, "Eroare la crearea socket-ului: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Convert IP string to binary
    iResult = inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    if (iResult <= 0)
    {
        fprintf(stderr, "Adresa IP invalida: %s\n", server_ip);
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        fprintf(stderr, "Eroare la conectare: %ld\n", WSAGetLastError());
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }

    // Trimit nickname:parola la server dupa conectare
    char auth_data[BUFFER_SIZE * 2];
    snprintf(auth_data, sizeof(auth_data), "%s:%s", nickname, password);
    send(socket_fd, auth_data, (int)strlen(auth_data), 0);

    // Astept confirmarea de la server
    char response[BUFFER_SIZE];
    int bytes = recv(socket_fd, response, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        fprintf(stderr, "Conexiune inchisa de server\n");
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }
    response[bytes] = '\0';
    if (strstr(response, "reusita") == NULL) {
        fprintf(stderr, "Autentificare esuata.\n");
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }
    printf("%s", response);

    printf("Conectat la server %s:%d ca '%s'\n", server_ip, server_port, nickname);

    // Creez thread-uri pentru primire si trimitere mesaje
    HANDLE recv_thread = CreateThread(NULL, 0, recv_thread_func, NULL, 0, NULL);
    HANDLE send_thread = CreateThread(NULL, 0, send_thread_func, NULL, 0, NULL);

    // Astept thread-urile
    WaitForSingleObject(send_thread, INFINITE);
    running = 0;
    shutdown(socket_fd, SD_BOTH);
    WaitForSingleObject(recv_thread, INFINITE);

    // Inchid socket-ul si WinSock
    closesocket(socket_fd);
    WSACleanup();

    printf("Client închis.\n");
    return 0;
}
