#pragma once

CString PathAppend(CString path, LPCTSTR sub);
void PathRemoveArgs(CString &path);
void PathUnquoteSpaces(CString &path);
CString PathFindExtension(CString path);
void PathRemoveFileSpec(CString &path);