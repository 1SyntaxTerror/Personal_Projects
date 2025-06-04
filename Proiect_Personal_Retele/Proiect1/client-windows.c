// client-windows-thread.c
#define _WIN32_WINNT 0x0601 // Windows 7 sau mai nou, pentru funcții threads

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "9091"
#define DEFAULT_BUFLEN 1024

HANDLE hThreadRecv;

SOCKET ConnectSocket = INVALID_SOCKET;
volatile int running = 1;

DWORD WINAPI recvThreadFunc(LPVOID lpParam) {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;

    while (running) {
        memset(recvbuf, 0, recvbuflen);
        iResult = recv(ConnectSocket, recvbuf, recvbuflen - 1, 0);
        if (iResult > 0) {
            printf("\n%s\n> ", recvbuf);
            fflush(stdout);
        } else if (iResult == 0) {
            printf("\nConexiunea a fost închisă de server.\n");
            running = 0;
            break;
        } else {
            printf("\nEroare la recv: %d\n", WSAGetLastError());
            running = 0;
            break;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    WSADATA wsaData;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    char sendbuf[DEFAULT_BUFLEN];
    int iResult;

    char *serverName = NULL;
    char *serverPort = NULL;

    if (argc >= 2)
        serverName = argv[1];
    else
        serverName = "127.0.0.1";

    if (argc >= 3)
        serverPort = argv[2];
    else
        serverPort = DEFAULT_PORT;

    printf("Client TCP - conectare la %s:%s\n", serverName, serverPort);

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IPv4 sau IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Rezolvă adresa serverului
    iResult = getaddrinfo(serverName, serverPort, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Crează socket și încearcă să te conectezi
    for(ptr=result; ptr != NULL ; ptr=ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("Socket creation failed: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    printf("Conectat la server! Introdu numele tau: ");
    char nickname[64];
    fgets(nickname, sizeof(nickname), stdin);
    // elimină \n
    nickname[strcspn(nickname, "\r\n")] = 0;

    // Trimite nickname-ul la server
    send(ConnectSocket, nickname, (int)strlen(nickname), 0);

    // Pornim thread-ul pentru citirea mesajelor de la server
    DWORD threadId;
    hThreadRecv = CreateThread(NULL, 0, recvThreadFunc, NULL, 0, &threadId);
    if (hThreadRecv == NULL) {
        printf("Eroare la crearea thread-ului de recv\n");
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Loop de trimitere mesaje
    printf("Scrie mesaj (\"exit\" pentru a iesi):\n> ");
    while (running && fgets(sendbuf, DEFAULT_BUFLEN, stdin) != NULL) {
        // elimina newline
        sendbuf[strcspn(sendbuf, "\r\n")] = 0;

        if (strcmp(sendbuf, "exit") == 0) {
            running = 0;
            break;
        }

        iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
        if (iResult == SOCKET_ERROR) {
            printf("Eroare la send: %d\n", WSAGetLastError());
            running = 0;
            break;
        }
        printf("> ");
    }

    // Inchidem conexiunea si curatam Winsock
    shutdown(ConnectSocket, SD_BOTH);
    closesocket(ConnectSocket);

    WaitForSingleObject(hThreadRecv, INFINITE);
    CloseHandle(hThreadRecv);

    WSACleanup();

    printf("Clientul s-a inchis.\n");
    return 0;
}
