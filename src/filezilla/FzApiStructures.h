
#pragma once

#include <nbsystem.h>
#include <openssl/pkcs12.h>
#include <FileBuffer.h>

class t_server
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  t_server();
  ~t_server();
  CString host;
  int port;
  CString user, pass, account;
  CString path;
  int nServerType;
  int nPasv;
  int nTimeZoneOffset;
  int nUTF8;
  int nCodePage;
  int iForcePasvIp;
  int iUseMlsd;
  int iDupFF;
  int iUndupFF;
  bool operator<(const t_server &op) const; //Needed by STL map
  X509 * Certificate;
  EVP_PKEY * PrivateKey;
};

const bool operator==(const t_server &a,const t_server &b);
const bool operator!=(const t_server &a,const t_server &b);

#include "ServerPath.h"

struct t_transferfile
{
CUSTOM_MEM_ALLOCATION_IMPL

    CString localfile;
    CString remotefile;
    CServerPath remotepath;
    BOOL get;
    int64_t size;
    t_server server;
    int nType;
    void * nUserData;
    TTransferOutEvent OnTransferOut;
    TTransferInEvent OnTransferIn;
};

