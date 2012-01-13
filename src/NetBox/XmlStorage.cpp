//---------------------------------------------------------------------------
#include "stdafx.h"

#include <vector>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "XmlStorage.h"
#include "TextsCore.h"
#include "FarUtil.h"
#include "version.h"
//---------------------------------------------------------------------------
static const char *CONST_XML_VERSION = "2.0";
static const char *CONST_ROOT_NODE = "NetBox";
//---------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch(...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch(...) { FFailed++; }

//===========================================================================
TXmlStorage::TXmlStorage(const std::wstring &AStorage) :
  THierarchicalStorage(ExcludeTrailingBackslash(AStorage)),
  FXmlDoc(NULL),
  FCurrentElement(NULL)
{
  Init();
};
//---------------------------------------------------------------------------
void TXmlStorage::Init()
{
    FFailed = 0;
    FXmlDoc = new TiXmlDocument();
    FXmlDoc->LinkEndChild(new TiXmlDeclaration(CONST_XML_VERSION, "UTF-8", ""));
    FCurrentElement = new TiXmlElement(CONST_ROOT_NODE);
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
  bool Result = true;
  /*
  TRegistry * Registry = Storage->FRegistry;
  TStrings * Names = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    Registry->GetValueNames(Names);
    std::vector<unsigned char> Buffer(1024, 0);
    size_t Index = 0;
    while ((Index < Names->GetCount()) && Result)
    {
      std::wstring Name = PuttyMungeStr(Names->GetString(Index));
      unsigned long Size = Buffer.size();
      unsigned long Type;
      int RegResult;
      do
      {
        RegResult = RegQueryValueEx(Registry->GetCurrentKey(), Name.c_str(), NULL,
          &Type, &Buffer[0], &Size);
        if (RegResult == ERROR_MORE_DATA)
        {
          Buffer.resize(Size);
        }
      } while (RegResult == ERROR_MORE_DATA);

      Result = (RegResult == ERROR_SUCCESS);
      if (Result)
      {
        RegResult = RegSetValueEx(FRegistry->GetCurrentKey(), Name.c_str(), NULL, Type,
          &Buffer[0], Size);
        Result = (RegResult == ERROR_SUCCESS);
      }

      ++Index;
    }
  }
  */
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
  if (FXmlDoc)
  {
    switch (GetAccessMode()) {
      case smRead:
        // FRegistry->SetAccess(KEY_READ);
        break;

      case smReadWrite:
      default:
        // FRegistry->SetAccess(KEY_READ | KEY_WRITE);
        break;
    }
  }
}
//---------------------------------------------------------------------------
bool TXmlStorage::OpenSubKey(const std::wstring &SubKey, bool CanCreate, bool Path)
{
  /*
  // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
  // if (FKeyHistory->GetCount() > 0) FRegistry->CloseKey();
  std::wstring K = ExcludeTrailingBackslash(GetStorage() + GetCurrentSubKey() + MungeSubKey(SubKey, Path));
  bool Result = false; // FRegistry->OpenKey(K, CanCreate);
  if (Result) Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
  return Result;
  */
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
        // root = FPluginSettings.CreateSubKey(FRoot, SubKey.c_str());
        std::string str = ToStdString(SubKey);
        Element = new TiXmlElement(str);
        // xmlNode->LinkEndChild(new TiXmlText(str));
        FCurrentElement->LinkEndChild(Element);
      }
      else
      {
        // root = FPluginSettings.OpenSubKey(FRoot, SubKey.c_str());
        Element = NULL; // new TiXmlElement(ToStdString(SubKey));
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
    }
  }
  // DEBUG_PRINTF(L"end, Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::CloseSubKey()
{
  // FRegistry->CloseKey();
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
  // ::Error(SNotImplemented, 3023);
  bool result = false;
  TiXmlElement *Element = FindElement(Name);
  if (Element != NULL)
  {
    FCurrentElement->RemoveChild(Element);
    result = true;
  }
  return result; // FRegistry->DeleteValue(Name);
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
  /*
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      FRegistry->ReadBinaryData(Name, &Result, sizeof(Result));
    }
    catch(...)
    {
      FFailed++;
    }
  }
  */
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
  /*
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      Result = FRegistry->ReadBinaryData(Name, Buffer, Size);
    }
    catch(...)
    {
      Result = 0;
      FFailed++;
    }
  }
  else
  {
    Result = 0;
  }
  */
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
