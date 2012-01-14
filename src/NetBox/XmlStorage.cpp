//---------------------------------------------------------------------------
#include "stdafx.h"

#include <vector>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "XmlStorage.h"
#include "TextsCore.h"
#include "FarUtil.h"
//---------------------------------------------------------------------------
static const char *CONST_XML_VERSION = "2.0";
static const char *CONST_ROOT_NODE = "NetBox";
static const char *CONST_SESSION_NODE = "Session";
static const char *CONST_VERSION_ATTR = "version";
static const char *CONST_NAME_ATTR = "name";
//---------------------------------------------------------------------------
TXmlStorage::TXmlStorage(const std::wstring &AStorage,
    const std::wstring &StoredSessionsSubKey) :
    THierarchicalStorage(ExcludeTrailingBackslash(AStorage)),
    FXmlDoc(NULL),
    FCurrentElement(NULL),
    FStoredSessionsSubKey(StoredSessionsSubKey),
    FFailed(0),
    FStoredSessionsOpened(false)
{
};
//---------------------------------------------------------------------------
void TXmlStorage::Init()
{
    THierarchicalStorage::Init();
    FXmlDoc = new TiXmlDocument();
    FXmlDoc->LinkEndChild(new TiXmlDeclaration("1.0", "UTF-8", ""));
    FCurrentElement = new TiXmlElement(CONST_ROOT_NODE);
    FCurrentElement->SetAttribute(CONST_VERSION_ATTR, CONST_XML_VERSION);
    FXmlDoc->LinkEndChild(FCurrentElement);
}
//---------------------------------------------------------------------------
TXmlStorage::~TXmlStorage()
{
    SaveXml();
    delete FXmlDoc;
};
//---------------------------------------------------------------------------
bool TXmlStorage::SaveXml()
{
    TiXmlPrinter xmlPrinter;
    xmlPrinter.SetIndent("  ");
    xmlPrinter.SetLineBreak("\r\n");
    FXmlDoc->Accept(&xmlPrinter);
    const char *xmlContent = xmlPrinter.CStr();
    if (!xmlContent || !*xmlContent)
    {
        return false;
    }

    return (CNBFile::SaveFile(GetStorage().c_str(), xmlContent) == ERROR_SUCCESS);
}
//---------------------------------------------------------------------------
bool TXmlStorage::Copy(TXmlStorage * Storage)
{
  ::Error(SNotImplemented, 3020);
  bool Result = false;
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TXmlStorage::GetSource()
{
  return GetStorage();
}
//---------------------------------------------------------------------------
void TXmlStorage::SetAccessMode(TStorageAccessMode value)
{
  THierarchicalStorage::SetAccessMode(value);
}
//---------------------------------------------------------------------------
bool TXmlStorage::OpenSubKey(const std::wstring &SubKey, bool CanCreate, bool Path)
{
  // DEBUG_PRINTF(L"SubKey = %s, CanCreate = %d, Path = %d", SubKey.c_str(), CanCreate, Path);
  TiXmlElement *OldCurrentElement = FCurrentElement;
  TiXmlElement *Element = NULL;
  if (Path)
  {
    std::wstring subKey = SubKey;
    assert(subKey.empty() || (subKey[subKey.size() - 1] != '\\'));
    bool Result = true;
    while (!subKey.empty())
    {
      Result &= OpenSubKey(CutToChar(subKey, L'\\', false), CanCreate, false);
      // DEBUG_PRINTF(L"SubKey = %s, Result = %d", SubKey.c_str(), Result);
    }
    return Result;
  }
  else
  {
      if (CanCreate)
      {
        std::string subKey = ToStdString(SubKey);
        if (FStoredSessionsOpened)
        {
            Element = new TiXmlElement(CONST_SESSION_NODE);
            Element->SetAttribute(CONST_NAME_ATTR, subKey);
        }
        else
        {
            Element = new TiXmlElement(subKey);
        }
        FCurrentElement->LinkEndChild(Element);
      }
      else
      {
        Element = NULL;
      }
  }
  bool Result = Element != NULL;
  if (Result)
  {
    Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
    if (Result)
    {
        FSubElements.push_back(OldCurrentElement);
        FCurrentElement = Element;
        FStoredSessionsOpened = (SubKey == FStoredSessionsSubKey);
    }
  }
  // DEBUG_PRINTF(L"end, Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::CloseSubKey()
{
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount() && FSubElements.size())
  {
    FCurrentElement = FSubElements.back();
    FSubElements.pop_back();
  }
  else
  {
    FCurrentElement = NULL;
  }
}
//---------------------------------------------------------------------------
bool TXmlStorage::DeleteSubKey(const std::wstring &SubKey)
{
  std::wstring K;
  if (FKeyHistory->GetCount() == 0) K = GetStorage() + GetCurrentSubKey();
  K += PuttyMungeStr(SubKey);
  return false; // FRegistry->DeleteKey(K);
}
//---------------------------------------------------------------------------
void TXmlStorage::GetSubKeyNames(TStrings* Strings)
{
  ::Error(SNotImplemented, 3021);
  // FRegistry->GetKeyNames(Strings);
  for (size_t Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->PutString(Index, PuttyUnMungeStr(Strings->GetString(Index)));
  }
}
//---------------------------------------------------------------------------
void TXmlStorage::GetValueNames(TStrings* Strings)
{
  ::Error(SNotImplemented, 3022);
  // FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TXmlStorage::DeleteValue(const std::wstring &Name)
{
  bool result = false;
  TiXmlElement *Element = FindElement(Name);
  if (Element != NULL)
  {
    FCurrentElement->RemoveChild(Element);
    result = true;
  }
  return result;
}
//---------------------------------------------------------------------------
bool TXmlStorage::KeyExists(const std::wstring &SubKey)
{
  ::Error(SNotImplemented, 3024);
  std::wstring K = PuttyMungeStr(SubKey);
  bool Result = false; // FRegistry->KeyExists(K);
  return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::RemoveIfExists(const std::wstring &Name)
{
    TiXmlElement *Element = FindElement(Name);
    if (Element != NULL)
    {
        FCurrentElement->RemoveChild(Element);
    }
}
//---------------------------------------------------------------------------
void TXmlStorage::AddNewElement(const std::wstring &Name, const std::wstring &Value)
{
    std::string name = ToStdString(Name);
    std::string value = ToStdString(Value);
    TiXmlElement *Element = new TiXmlElement(name);
    Element->LinkEndChild(new TiXmlText(value.c_str()));
    FCurrentElement->LinkEndChild(Element);
}
//---------------------------------------------------------------------------
TiXmlElement *TXmlStorage::FindElement(const std::wstring &Value)
{
  for (const TiXmlElement *Element = FCurrentElement->FirstChildElement();
    Element != NULL; Element = FCurrentElement->NextSiblingElement())
  {
    std::wstring val = ToStdWString(Element->ValueStr());
    DEBUG_PRINTF(L"val = %s", val.c_str());
    if (val == Value)
    {
        return const_cast<TiXmlElement *>(Element);
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
bool TXmlStorage::ValueExists(const std::wstring &Value)
{
    // ::Error(SNotImplemented, 3025);
    bool result = false;
    TiXmlElement *Element = FindElement(Value);
    if (Element != NULL)
    {
        FCurrentElement->RemoveChild(Element);
        result = true;
    }
    return result;
}
//---------------------------------------------------------------------------
int TXmlStorage::BinaryDataSize(const std::wstring &Name)
{
  ::Error(SNotImplemented, 3026);
  int Result = 0; // FRegistry->GetDataSize(Name);
  return Result;
}
//---------------------------------------------------------------------------
bool TXmlStorage::Readbool(const std::wstring &Name, bool Default)
{
  ::Error(SNotImplemented, 3027);
  // READ_REGISTRY(Readbool);
  return false;
}
//---------------------------------------------------------------------------
TDateTime TXmlStorage::ReadDateTime(const std::wstring &Name, TDateTime Default)
{
  // READ_REGISTRY(ReadDateTime);
  return TDateTime();
}
//---------------------------------------------------------------------------
double TXmlStorage::ReadFloat(const std::wstring &Name, double Default)
{
  // READ_REGISTRY(ReadFloat);
  return 0.0;
}
//---------------------------------------------------------------------------
int TXmlStorage::Readint(const std::wstring &Name, int Default)
{
  // READ_REGISTRY(Readint);
  return 0;
}
//---------------------------------------------------------------------------
__int64 TXmlStorage::ReadInt64(const std::wstring &Name, __int64 Default)
{
  __int64 Result = Default;
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TXmlStorage::ReadStringRaw(const std::wstring &Name, const std::wstring &Default)
{
  // READ_REGISTRY(ReadString);
  return L"";
}
//---------------------------------------------------------------------------
int TXmlStorage::ReadBinaryData(const std::wstring &Name,
  void * Buffer, int Size)
{
  ::Error(SNotImplemented, 3028);
  int Result = 0;
  return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::Writebool(const std::wstring &Name, bool Value)
{
  WriteString(Name, ::BooleanToEngStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteDateTime(const std::wstring &Name, TDateTime Value)
{
  WriteFloat(Name, Value);
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteFloat(const std::wstring &Name, double Value)
{
    RemoveIfExists(Name);
    AddNewElement(Name, FORMAT(L"%.5f", Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteStringRaw(const std::wstring &Name, const std::wstring &Value)
{
    RemoveIfExists(Name);
    AddNewElement(Name, Value);
}
//---------------------------------------------------------------------------
void TXmlStorage::Writeint(const std::wstring &Name, int Value)
{
    RemoveIfExists(Name);
    AddNewElement(Name, ::IntToStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteInt64(const std::wstring &Name, __int64 Value)
{
    RemoveIfExists(Name);
    AddNewElement(Name, ::Int64ToStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteBinaryData(const std::wstring &Name,
  const void * Buffer, int Size)
{
    RemoveIfExists(Name);
    AddNewElement(Name, ::StrToHex(std::wstring(reinterpret_cast<const wchar_t *>(Buffer), Size), true));
}
//---------------------------------------------------------------------------
int TXmlStorage::GetFailed()
{
  int Result = FFailed;
  FFailed = 0;
  return Result;
}
