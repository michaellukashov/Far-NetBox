/*CAsyncSocketEx by Tim Kosse (Tim.Kosse@gmx.de)
            Version 1.1 (2002-11-01)
--------------------------------------------------------

Introduction:
-------------

CAsyncSocketEx is a replacement for the MFC class CAsyncSocket.
This class was written because CAsyncSocket is not the fastest WinSock
wrapper and it's very hard to add new functionality to CAsyncSocket
derived classes. This class offers the same functionality as CAsyncSocket.
Also, CAsyncSocketEx offers some enhancements which were not possible with
CAsyncSocket without some tricks.

How do I use it?
----------------
Basically exactly like CAsyncSocket.
To use CAsyncSocketEx, just replace all occurrences of CAsyncSocket in your
code with CAsyncSocketEx, if you did not enhance CAsyncSocket yourself in
any way, you won't have to change anything else in your code.

Why is CAsyncSocketEx faster?
-----------------------------

CAsyncSocketEx is slightly faster when dispatching notification event messages.
First have a look at the way CAsyncSocket works. For each thread that uses
CAsyncSocket, a window is created. CAsyncSocket calls WSAAsyncSelect with
the handle of that window. Until here, CAsyncSocketEx works the same way.
But CAsyncSocket uses only one window message (WM_SOCKET_NOTIFY) for all
sockets within one thread. When the window receive WM_SOCKET_NOTIFY, wParam
contains the socket handle and the window looks up an CAsyncSocket instance
using a map. CAsyncSocketEx works differently. It's helper window uses a
wide range of different window messages (WM_USER through 0xBFFF) and passes
a different message to WSAAsyncSelect for each socket. When a message in
the specified range is received, CAsyncSocketEx looks up the pointer to a
CAsyncSocketEx instance in an Array using the index of message - WM_USER.
As you can see, CAsyncSocketEx uses the helper window in a more efficient
way, as it don't have to use the slow maps to lookup it's own instance.
Still, speed increase is not very much, but it may be noticeable when using
a lot of sockets at the same time.
Please note that the changes do not affect the raw data throughput rate,
CAsyncSocketEx only dispatches the notification messages faster.

What else does CAsyncSocketEx offer?
------------------------------------

CAsyncSocketEx offers a flexible layer system. One example is the proxy layer.
Just create an instance of the proxy layer, configure it and add it to the layer
chain of your CAsyncSocketEx instance. After that, you can connect through
proxies.
Benefit: You don't have to change much to use the layer system.
Another layer that is currently in development is the SSL layer to establish
SSL encrypted connections.

License
-------

Feel free to use this class, as long as you don't claim that you wrote it
and this copyright notice stays intact in the source files.
If you use this class in commercial applications, please send a short message
to tim.kosse@gmx.de
*/

#pragma once

#include "AsyncSocketEx.h"

class CAsyncSocketEx;

class CAsyncSocketExLayer
{
CUSTOM_MEM_ALLOCATION_IMPL
  friend class CAsyncSocketEx;
  friend class CAsyncSocketExHelperWindow;
protected:
  // Protected constructor so that CAsyncSocketExLayer can't be instantiated
  CAsyncSocketExLayer();
  virtual ~CAsyncSocketExLayer();

  // Notification event handlers
  virtual void OnAccept(int nErrorCode);
  virtual void OnClose(int nErrorCode);
  virtual void OnConnect(int nErrorCode);
  virtual void OnReceive(int nErrorCode);
  virtual void OnSend(int nErrorCode);

  // Operations
  virtual BOOL Accept(CAsyncSocketEx & rConnectedSocket, SOCKADDR * lpSockAddr = nullptr, int * lpSockAddrLen = nullptr);
  virtual void Close();
  virtual BOOL Connect(LPCTSTR lpszHostAddress, UINT nHostPort);
  virtual BOOL Connect(const SOCKADDR * lpSockAddr, int nSockAddrLen);
  virtual BOOL Create(UINT nSocketPort = 0, int nSocketType = SOCK_STREAM,
     long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE,
     LPCTSTR lpszSocketAddress = nullptr, int nFamily = AF_INET);
  virtual BOOL GetPeerName(SOCKADDR * lpSockAddr, int * lpSockAddrLen);
  virtual BOOL GetSockName(SOCKADDR * lpSockAddr, int * lpSockAddrLen);
  virtual BOOL GetPeerName(CString& rPeerAddress, UINT& rPeerPort);
  virtual BOOL GetSockName(CString& rPeerAddress, UINT& rPeerPort);
  virtual BOOL Listen(int nConnectionBacklog);
  virtual int Receive(void * lpBuf, int nBufLen, int nFlags = 0);
  virtual int Send(const void * lpBuf, int nBufLen, int nFlags = 0);
  virtual BOOL ShutDown(int nHow = sends);
  enum { receives = 0, sends = 1, both = 2 };

  // Functions that will call next layer
  BOOL ShutDownNext(int nHow = sends);
  BOOL AcceptNext(CAsyncSocketEx& rConnectedSocket, SOCKADDR * lpSockAddr = nullptr, int * lpSockAddrLen = nullptr);
  void CloseNext();
  BOOL ConnectNext(LPCTSTR lpszHostAddress, UINT nHostPort);
  BOOL ConnectNext( const SOCKADDR * lpSockAddr, int nSockAddrLen);
  BOOL CreateNext(UINT nSocketPort, int nSocketType, long lEvent, LPCTSTR lpszSocketAddress, int nFamily = AF_INET);
  BOOL GetPeerNameNext(SOCKADDR * lpSockAddr, int* lpSockAddrLen);
  BOOL GetSockNameNext(SOCKADDR * lpSockAddr, int* lpSockAddrLen);
  BOOL GetPeerNameNext(CString & rPeerAddress, UINT& rPeerPort);
  BOOL GetSockNameNext(CString & rPeerAddress, UINT& rPeerPort);
  BOOL ListenNext(int nConnectionBacklog);
  int32_t ReceiveNext(void * lpBuf, int nBufLen, int nFlags = 0);
  int32_t SendNext(const void * lpBuf, int nBufLen, int nFlags = 0);

  CAsyncSocketEx *m_pOwnerSocket{nullptr};
  int32_t m_nCriticalError;

  // Calls OnLayerCallback on owner socket
  int DoLayerCallback(int nType, int32_t nParam1, intptr_t nParam2, char * str = 0);

  int GetLayerState();
  BOOL TriggerEvent(long lEvent, int nErrorCode, BOOL bPassThrough = FALSE);

  // Gets the socket family
  int GetFamily() const;

  // Sets the socket family
  bool SetFamily(int nFamily);

  // Iterate through protocols
  bool TryNextProtocol();

  void LogSocketMessageRaw(int nMessageType, LPCTSTR pMsg);
  bool LoggingSocketMessage(int nMessageType);
  int GetSocketOptionVal(int OptionID) const;

private:
  // Layer state can't be set directly from derived classes
  void SetLayerState(int nLayerState);
  int m_nLayerState{0};

  int m_nFamily{0};
  int m_lEvent{0};
  LPTSTR m_lpszSocketAddress{nullptr};
  UINT m_nSocketPort{0};

  addrinfo * m_addrInfo{nullptr}, * m_nextAddr{nullptr};

  // Called by helper window, dispatches event notification and updated layer state
  void CallEvent(int nEvent, int nErrorCode);

  int m_nPendingEvents{0};

  void Init(CAsyncSocketExLayer * pPrevLayer, CAsyncSocketEx * pOwnerSocket);
  CAsyncSocketExLayer * AddLayer(CAsyncSocketExLayer * pLayer, CAsyncSocketEx * pOwnerSocket);

  CAsyncSocketExLayer * m_pNextLayer;
  CAsyncSocketExLayer * m_pPrevLayer;

  struct t_LayerNotifyMsg
  {
  CUSTOM_MEM_ALLOCATION_IMPL
    SOCKET hSocket{};
    CAsyncSocketExLayer * pLayer{nullptr};
    long lEvent{0};
  };
};

