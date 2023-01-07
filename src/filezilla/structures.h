//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include <nbsystem.h>
//---------------------------------------------------------------------------
class CServerPath;
//---------------------------------------------------------------------------
class t_directory //: public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  t_directory();
  t_directory(const t_directory &a);
  ~t_directory();
  t_server server;
  CServerPath path;
  int num;
  class t_direntry // : public TObject
  {
  CUSTOM_MEM_ALLOCATION_IMPL
  public:
    t_direntry();
    CString linkTarget;
    CString name;
    CString permissionstr;
    CString humanpermstr; // RFC format
    CString ownergroup; // deprecated, to be replaced with owner/group
    CString owner;
    CString group;
    int64_t size;
    bool bUnsure; // Set by CFtpControlSocket::FileTransfer when uploads fail after sending STOR/APPE
    bool dir;
    bool bLink;
    class t_date // : public TObject
    {
    CUSTOM_MEM_ALLOCATION_IMPL
    public:
      t_date();
      int year,month,day,hour,minute,second;
      bool hastime;
      bool hasyear; // ignored and assumed true when hasseconds
      bool hasseconds;
      bool hasdate;
      bool utc;
    } date;
  } * direntry;
  t_directory & operator=(const t_directory & a);
};

