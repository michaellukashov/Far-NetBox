#include "stdafx.h"


#if defined(__BORLANDC__)
AFX_COMDAT int _afxInitDataA[] = { -1, 0, 0, 0 };
AFX_COMDAT CStringDataA* _afxDataNilA = (CStringDataA*)&_afxInitDataA;
AFX_COMDAT LPCSTR _afxPchNilA = (LPCSTR)(((BYTE*)&_afxInitDataA)+sizeof(CStringDataA));
#endif

t_directory::t_directory() :
  num(0),
  direntry(0)
{
#ifndef MPEXT_NO_CACHE
  bCached=FALSE;
#endif
}

t_directory::t_directory(const t_directory & a) :
  num(0),
  direntry(0)
{
  this->operator=(a);
}

t_directory::~t_directory()
{
  if (direntry)
    delete [] direntry;
}

t_directory& t_directory::operator=(const t_directory &a)
{
  if (&a==this)
    return *this;

  if (direntry)
    delete [] direntry;
  direntry=0;
  path=a.path;
  num=a.num;
  server=a.server;
  if (num)
    direntry=new t_directory::t_direntry[num];
  for (int i=0;i<num;i++)
    direntry[i]=a.direntry[i];
#ifndef MPEXT_NO_CACHE
  bCached=a.bCached;
#endif
  return *this;
}

t_directory::t_direntry::t_direntry()
{
  dir=FALSE;
  size=0;
  bUnsure=TRUE;
  bLink=FALSE;
  EntryTime=CTime::GetCurrentTime();
}

t_directory::t_direntry::t_date::t_date()
{
  year=month=day=hour=minute=second=0;
  hasdate=hastime=hasseconds=utc=FALSE;
}

void t_directory::Merge(const t_directory &directory, CTime MergeTime)
{
  rde::list<t_direntry> AddList;
  if (!num)
  {
    if (!directory.num)
       return;
    for (int i=0; i<directory.num; i++)
      if (directory.direntry[i].EntryTime>=MergeTime)
        AddList.push_back(directory.direntry[i]);
  }
  else
  {
    DebugAssert(num>0 && directory.num>=0);
    int i;
    CTime oldestTime=CTime::GetCurrentTime();
    for (i=0; i<num; i++)
      if (direntry[i].EntryTime<oldestTime)
        oldestTime=direntry[i].EntryTime;
      
    for (i=0;i<directory.num;i++)
    {
      int j;
      for (j=0; j<num; j++)
      {
        if (directory.direntry[i].lName==direntry[j].lName)
        {
          if (directory.direntry[i].dir != direntry[j].dir)
            break;
          
          if (directory.direntry[i].EntryTime > direntry[j].EntryTime)
            direntry[j]=directory.direntry[i];
          break;
        }
      }
      if (j==num && directory.direntry[i].EntryTime>=oldestTime)
        AddList.push_back(directory.direntry[i]);
    }
  }
  
  if (AddList.empty())
    return;
  t_direntry *tmp=direntry;
  direntry=new t_direntry[num + AddList.size()];
  
  int i;
  for (i=0; i<num; i++)
    direntry[i] = tmp[i];

  for (rde::list<t_direntry>::iterator iter=AddList.begin(); iter!=AddList.end(); ++iter, i++)
    direntry[i] = *iter;

  num+=(int)AddList.size();

  delete [] tmp;
}
