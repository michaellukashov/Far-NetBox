#include "stdafx.h"
#include "pathfunctions.h"

void PathRemoveArgs(CString &path)
{
	path.TrimLeft( _T(" ") );
	path.TrimRight( _T(" ") );
	if (path==_MPT(""))
		return;
	BOOL quoted=FALSE;
	if (path[0]==_MPT('\"'))
		quoted=TRUE;
	int pos=path.ReverseFind(_MPT('\\'));
	if (pos==-1)
		pos=quoted?1:0;

	int i;
	for (i=pos;i<path.GetLength();i++)
	{
		if (path[i]==_MPT('\"'))
			break;
		if (path[i]==_MPT(' ') && !quoted)
			break;
	}
	path = path.Left(i+1);
	path.TrimRight(_MPT(' '));
}

void PathUnquoteSpaces(CString &path)
{
	int pos;
	while ((pos=path.Find(_MPT('\"')))!=-1)
		path.SetAt(pos,_MPT(' '));
	path.TrimLeft( _T(" ") );
	path.TrimRight( _T(" ") );
}

CString PathFindExtension(CString path)
{
	int pos=path.ReverseFind(_MPT('.'));
	if (pos==-1)
		return _MPT("");
	return path.Mid(pos);
}

void PathRemoveFileSpec(CString &path)
{
	CFileStatus64 status;
	if (GetStatus64(path,status))
	{
		if (status.m_attribute&0x10)
		{
			path.TrimRight( _T("\\") );
			path=path+_MPT("\\");
			return;
		}
		else
			path.TrimRight( _T("\\") );
	}
	if (path.Right(1)!=_MPT("\\"))
	{
		path.TrimRight( _T("\\") );
		int pos=path.ReverseFind(_MPT('\\'));
		if (pos==-1)
			path=_MPT("");
		else
			path=path.Left(pos+1);
	}
}

CString PathAppend(CString path, LPCTSTR sub)
{
	DebugAssert(sub);

	if (path.Right(1) != _T("\\"))
		path += _T("\\");
	path += sub;

	return path;
}
