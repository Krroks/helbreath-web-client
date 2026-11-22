// Winsock names XSocket.h needs in the Emscripten build (the sockets themselves are WebSockets, see Net/XSocket_web.cpp).
#pragma once
#include "windows.h"
#define INVALID_SOCKET ((SOCKET)(~0))
#define SOCKET_ERROR (-1)
#define FD_READ 0x01
#define FD_WRITE 0x02
#define FD_ACCEPT 0x08
#define FD_CONNECT 0x10
#define FD_CLOSE 0x20
#define WSAGETSELECTEVENT(l) LOWORD(l)
#define WSAGETSELECTERROR(l) HIWORD(l)
#define WSAMAKESELECTREPLY(e, err) MAKELONG(e, err)
typedef struct WSAData { WORD wVersion, wHighVersion; } WSADATA;
inline int WSAStartup(WORD, WSADATA *) { return 0; }
inline int WSACleanup() { return 0; }
