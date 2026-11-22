// XSocket over WebSocket, for the Emscripten build (replaces XSocket.cpp there).
// The page connects to ws(s)://<bridge>/<addr>:<port>; each WebSocket message carries exactly one frame
// [key][size u16][body], so a message maps to one FD_READ and one XSOCKEVENT_READCOMPLETE.
// Framing and cipher are copied from XSocket.cpp unchanged.
#include "XSocket.h"
#include <emscripten/websocket.h>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

// Routes a socket event like WndProc did (WebMain.cpp).
void hb_socket_event(unsigned int uiMsg, SOCKET sock, LPARAM lParam);

struct WsState {
    XSocket *owner;
    std::deque<std::vector<char>> frames;
};
static std::unordered_map<int, WsState> g_ws; // websocket handle -> state

EM_JS(char *, hbjs_ws_url, (const char *addr, int port), {
    const base = Module.hbBridgeUrl || ((location.protocol === 'https:' ? 'wss://' : 'ws://') + location.host + '/ws');
    return stringToNewUTF8(base + '/' + UTF8ToString(addr) + ':' + port);
});

static EM_BOOL on_open(int, const EmscriptenWebSocketOpenEvent *e, void *) {
    auto it = g_ws.find(e->socket);
    if (it == g_ws.end()) return EM_TRUE;
    hb_socket_event(it->second.owner->m_uiMsg, (SOCKET)e->socket, WSAMAKESELECTREPLY(FD_CONNECT, 0));
    return EM_TRUE;
}
static EM_BOOL on_message(int, const EmscriptenWebSocketMessageEvent *e, void *) {
    auto it = g_ws.find(e->socket);
    if (it == g_ws.end() || e->isText) return EM_TRUE;
    it->second.frames.emplace_back((const char *)e->data, (const char *)e->data + e->numBytes);
    hb_socket_event(it->second.owner->m_uiMsg, (SOCKET)e->socket, WSAMAKESELECTREPLY(FD_READ, 0));
    return EM_TRUE;
}
static EM_BOOL on_error(int, const EmscriptenWebSocketErrorEvent *e, void *) {
    auto it = g_ws.find(e->socket);
    if (it == g_ws.end()) return EM_TRUE;
    // before open an error means the connect failed, as WSAAsyncSelect reports it
    if (!it->second.owner->m_bIsAvailable)
        hb_socket_event(it->second.owner->m_uiMsg, (SOCKET)e->socket, WSAMAKESELECTREPLY(FD_CONNECT, 1));
    return EM_TRUE;
}
static EM_BOOL on_close(int, const EmscriptenWebSocketCloseEvent *e, void *) {
    auto it = g_ws.find(e->socket);
    if (it == g_ws.end() || !it->second.owner->m_bIsAvailable) return EM_TRUE;
    hb_socket_event(it->second.owner->m_uiMsg, (SOCKET)e->socket, WSAMAKESELECTREPLY(FD_CLOSE, 0));
    return EM_TRUE;
}

XSocket::XSocket(HWND hWnd, int iBlockLimit) {
    m_cType = NULL;
    m_pRcvBuffer = NULL;
    m_pSndBuffer = NULL;
    m_Sock = INVALID_SOCKET;
    m_dwBufferSize = 0;
    m_cStatus = XSOCKSTATUS_READINGHEADER;
    m_dwReadSize = 3;
    m_dwTotalReadSize = 0;
    for (int i = 0; i < XSOCKBLOCKLIMIT; i++) {
        m_iUnsentDataSize[i] = 0;
        m_pUnsentDataList[i] = NULL;
    }
    m_sHead = 0;
    m_sTail = 0;
    m_WSAErr = NULL;
    m_hWnd = hWnd;
    m_bIsAvailable = FALSE;
    m_bIsWriteEnabled = FALSE;
    m_iBlockLimit = iBlockLimit;
}

XSocket::~XSocket() {
    if (m_pRcvBuffer != NULL) delete[] m_pRcvBuffer;
    if (m_pSndBuffer != NULL) delete[] m_pSndBuffer;
    for (int i = 0; i < XSOCKBLOCKLIMIT; i++)
        if (m_pUnsentDataList[i] != NULL) delete[] m_pUnsentDataList[i];
    _CloseConn();
}

BOOL XSocket::bInitBufferSize(DWORD dwBufferSize) {
    if (m_pRcvBuffer != NULL) delete[] m_pRcvBuffer;
    if (m_pSndBuffer != NULL) delete[] m_pSndBuffer;
    m_pRcvBuffer = new char[dwBufferSize + 8];
    m_pSndBuffer = new char[dwBufferSize + 8];
    m_dwBufferSize = dwBufferSize;
    return TRUE;
}

BOOL XSocket::bConnect(char *pAddr, int iPort, unsigned int uiMsg) {
    if (m_cType == XSOCK_LISTENSOCK) return FALSE;
    _CloseConn();
    char *url = hbjs_ws_url(pAddr, iPort);
    EmscriptenWebSocketCreateAttributes attr;
    emscripten_websocket_init_create_attributes(&attr);
    attr.url = url;
    attr.protocols = NULL;
    EMSCRIPTEN_WEBSOCKET_T ws = emscripten_websocket_new(&attr);
    free(url);
    if (ws <= 0) return FALSE;
    g_ws[ws] = WsState{this, {}};
    emscripten_websocket_set_onopen_callback(ws, NULL, on_open);
    emscripten_websocket_set_onmessage_callback(ws, NULL, on_message);
    emscripten_websocket_set_onerror_callback(ws, NULL, on_error);
    emscripten_websocket_set_onclose_callback(ws, NULL, on_close);
    m_Sock = (SOCKET)ws;
    strcpy(m_pAddr, pAddr);
    m_iPortNum = iPort;
    m_uiMsg = uiMsg;
    m_cType = XSOCK_NORMALSOCK;
    m_bIsAvailable = FALSE;
    m_bIsWriteEnabled = FALSE;
    return TRUE;
}

BOOL XSocket::bBlockConnect(char *pAddr, int iPort, unsigned int uiMsg) { return bConnect(pAddr, iPort, uiMsg); }

int XSocket::iOnSocketEvent(WPARAM wParam, LPARAM lParam) {
    if (m_cType != XSOCK_NORMALSOCK) return XSOCKEVENT_SOCKETMISMATCH;
    if ((SOCKET)wParam != m_Sock) return XSOCKEVENT_SOCKETMISMATCH;
    switch (WSAGETSELECTEVENT(lParam)) {
    case FD_CONNECT:
        if (WSAGETSELECTERROR(lParam) != 0) {
            if (bConnect(m_pAddr, m_iPortNum, m_uiMsg) == FALSE) return XSOCKEVENT_SOCKETERROR;
            return XSOCKEVENT_RETRYINGCONNECTION;
        }
        m_bIsAvailable = TRUE;
        m_bIsWriteEnabled = TRUE;
        _iSendUnsentData();
        return XSOCKEVENT_CONNECTIONESTABLISH;
    case FD_READ:
        return _iOnRead();
    case FD_CLOSE:
        m_cType = XSOCK_SHUTDOWNEDSOCK;
        return XSOCKEVENT_SOCKETCLOSED;
    }
    return XSOCKEVENT_UNKNOWN;
}

int XSocket::_iOnRead() {
    auto it = g_ws.find((int)m_Sock);
    if (it == g_ws.end() || it->second.frames.empty()) return XSOCKEVENT_BLOCK;
    std::vector<char> f = std::move(it->second.frames.front());
    it->second.frames.pop_front();
    if (f.size() < 3) return XSOCKEVENT_SOCKETERROR;
    if (f.size() - 3 > m_dwBufferSize) return XSOCKEVENT_MSGSIZETOOLARGE;
    memcpy(m_pRcvBuffer, f.data(), f.size());
    return XSOCKEVENT_READCOMPLETE;
}

int XSocket::_iSend(char *Data, int iSize, BOOL bSaveFlag) {
    if (!m_bIsWriteEnabled) {
        if (!bSaveFlag) return XSOCKEVENT_BLOCK;
        int r = _iRegisterUnsentData(Data, iSize);
        return r == 1 ? XSOCKEVENT_BLOCK : r == 0 ? XSOCKEVENT_QUENEFULL : XSOCKEVENT_CRITICALERROR;
    }
    if (emscripten_websocket_send_binary((EMSCRIPTEN_WEBSOCKET_T)m_Sock, Data, iSize) != EMSCRIPTEN_RESULT_SUCCESS)
        return XSOCKEVENT_SOCKETERROR;
    return iSize;
}

int XSocket::_iSend_ForInternalUse(char *Data, int iSize) { return _iSend(Data, iSize, FALSE); }

int XSocket::_iRegisterUnsentData(char *Data, int iSize) {
    if (m_pUnsentDataList[m_sTail] != NULL) return 0;
    m_pUnsentDataList[m_sTail] = new char[iSize];
    memcpy(m_pUnsentDataList[m_sTail], Data, iSize);
    m_iUnsentDataSize[m_sTail] = iSize;
    m_sTail++;
    if (m_sTail >= m_iBlockLimit) m_sTail = 0;
    return 1;
}

int XSocket::_iSendUnsentData() {
    while (m_pUnsentDataList[m_sHead] != NULL) {
        if (_iSend(m_pUnsentDataList[m_sHead], m_iUnsentDataSize[m_sHead], FALSE) < 0) return XSOCKEVENT_UNSENTDATASENDBLOCK;
        delete[] m_pUnsentDataList[m_sHead];
        m_pUnsentDataList[m_sHead] = NULL;
        m_iUnsentDataSize[m_sHead] = 0;
        m_sHead++;
        if (m_sHead >= m_iBlockLimit) m_sHead = 0;
    }
    return XSOCKEVENT_UNSENTDATASENDCOMPLETE;
}

int XSocket::iSendMsg(char *Data, DWORD dwSize, char cKey) {
    WORD *wp;
    int i, iRet;
    if (dwSize > m_dwBufferSize) return XSOCKEVENT_MSGSIZETOOLARGE;
    if (m_cType != XSOCK_NORMALSOCK) return XSOCKEVENT_SOCKETMISMATCH;
    m_pSndBuffer[0] = cKey;
    wp = (WORD *)(m_pSndBuffer + 1);
    *wp = (WORD)(dwSize + 3);
    memcpy((char *)(m_pSndBuffer + 3), Data, dwSize);
    if (cKey != NULL) {
        for (i = 0; i < (int)(dwSize); i++) {
            m_pSndBuffer[3 + i] += (i ^ cKey);
            m_pSndBuffer[3 + i] = (char)(m_pSndBuffer[3 + i] ^ (cKey ^ (dwSize - i)));
        }
    }
    if (m_bIsWriteEnabled == FALSE) iRet = _iRegisterUnsentData(m_pSndBuffer, dwSize + 3);
    else iRet = _iSend(m_pSndBuffer, dwSize + 3, TRUE);
    if (iRet < 0) return iRet;
    else return (iRet - 3);
}

char *XSocket::pGetRcvDataPointer(DWORD *pMsgSize, char *pKey) {
    WORD *wp;
    DWORD dwSize;
    int i;
    char cKey;
    cKey = m_pRcvBuffer[0];
    if (pKey != NULL) *pKey = cKey;
    wp = (WORD *)(m_pRcvBuffer + 1);
    *pMsgSize = (*wp) - 3;
    dwSize = (*wp) - 3;
    if (cKey != NULL) {
        for (i = 0; i < (int)(dwSize); i++) {
            m_pRcvBuffer[3 + i] = (char)(m_pRcvBuffer[3 + i] ^ (cKey ^ (dwSize - i)));
            m_pRcvBuffer[3 + i] -= (i ^ cKey);
        }
    }
    return (m_pRcvBuffer + 3);
}

void XSocket::_CloseConn() {
    if (m_Sock == INVALID_SOCKET) return;
    g_ws.erase((int)m_Sock); // late callbacks for this socket are dropped
    emscripten_websocket_close((EMSCRIPTEN_WEBSOCKET_T)m_Sock, 1000, "");
    emscripten_websocket_delete((EMSCRIPTEN_WEBSOCKET_T)m_Sock);
    m_Sock = INVALID_SOCKET;
    m_cType = XSOCK_SHUTDOWNEDSOCK;
}

int XSocket::iGetPeerAddress(char *pAddrString) {
    strcpy(pAddrString, m_pAddr);
    return 0;
}

int XSocket::iSendMsgBlockingMode(char *buf, int nbytes) { return _iSend(buf, nbytes, TRUE); }
BOOL XSocket::bListen(char *, int, unsigned int) { return FALSE; }
BOOL XSocket::bAccept(class XSocket *, unsigned int) { return FALSE; }
