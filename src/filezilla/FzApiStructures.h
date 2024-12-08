
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
  int port{0};
  CString user, pass, account;
  CString path;
  int nServerType{0};
  int nPasv{0};
  int nTimeZoneOffset{0};
  int nUTF8{0};
  int nCodePage{0};
  int iForcePasvIp{0};
  int iUseMlsd{0};
  int iDupFF{0};
  int iUndupFF{0};
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
    BOOL get{FALSE};
    int64_t size{0};
    t_server server;
    int nType{0};
    void * nUserData{nullptr};
    TTransferOutEvent OnTransferOut;
    TTransferInEvent OnTransferIn;
};

