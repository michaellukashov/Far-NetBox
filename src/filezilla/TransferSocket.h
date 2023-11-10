
#pragma once

#include "FtpListResult.h"

#include "FtpControlSocket.h"
#include "ApiLog.h"

#ifndef MPEXT_NO_ZLIB
#include <zlib.h>
#endif

class CFtpControlSocket;
class CAsyncProxySocketLayer;
class CAsyncSslSocketLayer;
#ifndef MPEXT_NO_GSS
class CAsyncGssSocketLayer;
#endif

class CTransferSocket : public CAsyncSocketEx, public CApiLog
{
public:
  CFtpListResult * m_pListResult;

public:
  CTransferSocket(CFtpControlSocket * pOwner, int nMode);
  virtual ~CTransferSocket();

public:
  int m_nInternalMessageID;
  virtual void Close();
  virtual BOOL Create(BOOL bUseSsl);
  BOOL m_bListening{FALSE};
  CFile * m_pFile{nullptr};
  TTransferOutEvent m_OnTransferOut;
  TTransferInEvent m_OnTransferIn;
  t_transferdata m_transferdata;
  int64_t m_uploaded{0};
  void SetActive();
  int CheckForTimeout(int delay);
#ifndef MPEXT_NO_GSS
  void UseGSS(CAsyncGssSocketLayer * pGssLayer);
#endif
#ifndef MPEXT_NO_ZLIB
  bool InitZlib(int level);
#endif

public:
  virtual void OnReceive(int nErrorCode);
  virtual void OnAccept(int nErrorCode);
  virtual void OnConnect(int nErrorCode);
  virtual void OnClose(int nErrorCode);
  virtual void OnSend(int nErrorCode);
  virtual void SetState(int nState);

protected:
  virtual int OnLayerCallback(nb::list_t<t_callbackMsg> & callbacks);
  int ReadDataFromFile(char * buffer, int len);
  int ReadData(char * buffer, int len);
  void WriteData(const char * buffer, int len);
  virtual void LogSocketMessageRaw(int nMessageType, LPCTSTR pMsg);
  virtual int GetSocketOptionVal(int OptionID) const;
  virtual void ConfigureSocket();
  bool Activate();
  void Start();

  CFtpControlSocket * m_pOwner;
  CAsyncProxySocketLayer * m_pProxyLayer;
  CAsyncSslSocketLayer * m_pSslLayer;
#ifndef MPEXT_NO_GSS
  CAsyncGssSocketLayer * m_pGssLayer;
#endif
  void UpdateStatusBar(bool forceUpdate);
  BOOL m_bSentClose{FALSE};
  int m_bufferpos{0};
  char * m_pBuffer{nullptr};
#ifndef MPEXT_NO_ZLIB
  char * m_pBuffer2; // Used by zlib transfers
#endif
  BOOL m_bCheckTimeout{FALSE};
  CTime m_LastActiveTime;
  int m_nTransferState{0};
  int m_nMode{0};
  int m_nNotifyWaiting{0};
  bool m_bActivationPending{false};

  void CloseAndEnsureSendClose(int Mode);
  void EnsureSendClose(int Mode);
  void CloseOnShutDownOrError(int Mode);
  void SetBuffers();

  LARGE_INTEGER m_LastUpdateTime{};
  uint32_t m_LastSendBufferUpdate{0};
  DWORD m_SendBuf{0};

#ifndef MPEXT_NO_ZLIB
  z_stream m_zlibStream;
  bool m_useZlib;
#endif
};

