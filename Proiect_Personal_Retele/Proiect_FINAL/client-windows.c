#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_SIZE 1024

SOCKET sock = INVALID_SOCKET;
volatile int running = 1;

DWORD WINAPI recv_thread(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    while (running) {
        int bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived <= 0) {
            printf("\nConexiunea cu serverul a fost inchisă sau eroare.\n");
            running = 0;
            break;
        }
        buffer[bytesReceived] = '\0';
        printf("\n%s\n> ", buffer);
        fflush(stdout);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Folosește: %s <IP_SERVER> <PORT>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("Eroare la WSAStartup\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Eroare la crearea socketului\n");
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Eroare la conectare\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char nickname[BUFFER_SIZE];
    printf("Introdu numele tau: ");
    fgets(nickname, BUFFER_SIZE, stdin);
    size_t len = strlen(nickname);
    if (len > 0 && nickname[len - 1] == '\n') nickname[len - 1] = '\0';

    char password[BUFFER_SIZE];
    printf("Introdu parola ta: ");
    fgets(password, BUFFER_SIZE, stdin);
    len = strlen(password);
    if (len > 0 && password[len - 1] == '\n') password[len - 1] = '\0';

    char auth_data[BUFFER_SIZE * 2];
    snprintf(auth_data, sizeof(auth_data), "%s:%s", nickname, password);
    send(sock, auth_data, (int)strlen(auth_data), 0);

    char response[BUFFER_SIZE];
    int bytes = recv(sock, response, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        printf("Conexiune inchisa de server\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    response[bytes] = '\0';

    if (strstr(response, "Eroare") != NULL) {
        printf("%s\n", response);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    if (strstr(response, "reusita") == NULL) {
        printf("Autentificare esuata.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("%s\n", response);
    printf("Conectat la server %s:%d ca '%s'\n", server_ip, server_port, nickname);

    HANDLE hThread = CreateThread(NULL, 0, recv_thread, NULL, 0, NULL);
    if (hThread == NULL) {
        printf("Eroare la crearea thread-ului de receptionare\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char buffer[BUFFER_SIZE];
    while (running) {
        printf("> ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            running = 0;
            break;
        }

        len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            running = 0;
            shutdown(sock, SD_BOTH);
            break;
        }

        send(sock, buffer, (int)strlen(buffer), 0);
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    closesocket(sock);
    WSACleanup();

    printf("Client închis.\n");
    return 0;
}
