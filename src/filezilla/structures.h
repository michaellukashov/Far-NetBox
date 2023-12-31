
#pragma once

#include <nbsystem.h>

class CServerPath;

class t_directory //: public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  t_directory();
  t_directory(const t_directory &a);
  ~t_directory();
  CServerPath path;
  int num{0};
  class t_direntry // : public TObject
  {
  CUSTOM_MEM_ALLOCATION_IMPL
  public:
    t_direntry();
    bool bUnsure{false}; // Set by CFtpControlSocket::FileTransfer when uploads fail after sending STOR/APPE
    CString name;
    CString permissionstr;
    CString humanpermstr; // RFC format
    CString ownergroup; // deprecated, to be replaced with owner/group
    CString owner;
    CString group;
    int64_t size{0};
    bool dir{false};
    bool bLink{false};
    class t_date // : public TObject
    {
    CUSTOM_MEM_ALLOCATION_IMPL
    public:
      t_date();
      int year{0},month{0},day{0},hour{0},minute{0},second{0};
      bool hastime{false};
      bool hasyear{false}; // ignored and assumed true when hasseconds
      bool hasseconds{false};
      bool hasdate{false};
      bool utc{false};
    } date;
    CString linkTarget;
  } * direntry;
  t_server server;
  t_directory & operator=(const t_directory & a);
};

