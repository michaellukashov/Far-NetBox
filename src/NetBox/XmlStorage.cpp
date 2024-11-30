
#include <Classes.hpp>
#include <Common.h>
#include "XmlStorage.h"
#include "FarUtils.h"

constexpr const char * CONST_XML_VERSION21 = "2.1";
constexpr const char * CONST_ROOT_NODE = "NetBox";
constexpr const char * CONST_SESSION_NODE = "Session";
constexpr const char * CONST_VERSION_ATTR = "version";
constexpr const char * CONST_NAME_ATTR = "name";

TXmlStorage::TXmlStorage(const UnicodeString & AStorage,
  const UnicodeString & StoredSessionsSubKey) noexcept :
  THierarchicalStorage(::ExcludeTrailingBackslash(AStorage)),
  FStoredSessionsSubKey(StoredSessionsSubKey)
{
}

void TXmlStorage::Init()
{
  THierarchicalStorage::Init();
  // FXmlDoc = std::make_unique<tinyxml2::XMLDocument>();
}

TXmlStorage::~TXmlStorage() noexcept
{
  if (GetAccessMode() == smReadWrite)
  {
    WriteXml();
  }
  // SAFE_DESTROY_EX(tinyxml2::XMLDocument, FXmlDoc);
}

bool TXmlStorage::ReadXml()
{
  CNBFile xmlFile;
  if (!xmlFile.OpenRead(GetStorage().c_str()))
  {
    return false;
  }
  size_t BuffSize = nb::ToSizeT(xmlFile.GetFileSize() + 1);
  if (BuffSize > 1024 * 1024)
  {
    return false;
  }
  AnsiString Buff(nb::ToInt32(BuffSize), 0);
  if (!xmlFile.Read(&Buff[1], BuffSize))
  {
    return false;
  }

  FXmlDoc->Parse(Buff.c_str());
  if (FXmlDoc->Error())
  {
    return false;
  }

  // Get and check root node
  tinyxml2::XMLElement * xmlRoot = FXmlDoc->RootElement();
  if (!xmlRoot)
    return false;
  const char * Value = xmlRoot->Value();
  if (!Value)
    return false;
  if (strcmp(Value, CONST_ROOT_NODE) != 0)
    return false;
  const char * Attr = xmlRoot->Attribute(CONST_VERSION_ATTR);
  if (!Attr)
    return false;
  const uint32_t Version = ::StrToVersionNumber(UnicodeString(Attr));
  if (Version < MAKEVERSIONNUMBER(2, 0, 0))
    return false;
  const tinyxml2::XMLElement * Element = xmlRoot->FirstChildElement(AnsiString(FStoredSessionsSubKey).c_str());
  if (Element != nullptr)
  {
    FCurrentElement = FXmlDoc->RootElement();
    return true;
  }
  return false;
}

bool TXmlStorage::WriteXml() const
{
  tinyxml2::XMLPrinter xmlPrinter;
  // xmlPrinter.SetIndent("  ");
  // xmlPrinter.SetLineBreak("\r\n");
  FXmlDoc->Accept(&xmlPrinter);
  const char * xmlContent = xmlPrinter.CStr();
  if (!xmlContent || !*xmlContent)
  {
    return false;
  }

  return (CNBFile::SaveFile(GetStorage().c_str(), xmlContent) == ERROR_SUCCESS);
}

bool TXmlStorage::Copy(TXmlStorage * /*Storage*/)
{
  ThrowNotImplemented(3020);
  const bool Result = false;
  return Result;
}

void TXmlStorage::SetAccessModeProtected(TStorageAccessMode Value)
{
  THierarchicalStorage::SetAccessModeProtected(Value);
  switch (GetAccessMode())
  {
  case smRead:
    ReadXml();
    break;

  case smReadWrite:
  default:
    FXmlDoc->LinkEndChild(FXmlDoc->NewDeclaration());
    DebugAssert(FCurrentElement == nullptr);
    FCurrentElement = FXmlDoc->NewElement(CONST_ROOT_NODE);
    FCurrentElement->SetAttribute(CONST_VERSION_ATTR, CONST_XML_VERSION21);
    FXmlDoc->LinkEndChild(FCurrentElement);
    break;
  }
}

bool TXmlStorage::DoKeyExists(const UnicodeString & SubKey, bool /*ForceAnsi*/)
{
  const UnicodeString K = PuttyMungeStr(SubKey);
  const tinyxml2::XMLElement * Element = FindChildElement(AnsiString(K));
  const bool Result = Element != nullptr;
  return Result;
}

bool TXmlStorage::DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate)
{
  tinyxml2::XMLElement * OldCurrentElement = FCurrentElement;
  tinyxml2::XMLElement * Element = nullptr;
  const AnsiString SubKey(MungedSubKey);
  if (CanCreate)
  {
    if (FStoredSessionsOpened)
    {
      Element = FXmlDoc->NewElement(CONST_SESSION_NODE);
      Element->SetAttribute(CONST_NAME_ATTR, SubKey.c_str());
    }
    else
    {
      Element = FXmlDoc->NewElement(SubKey.c_str());
    }
    FCurrentElement->LinkEndChild(Element);
  }
  else
  {
    Element = FindChildElement(SubKey);
  }
  const bool Result = Element != nullptr;
  if (Result)
  {
    FSubElements.push_back(OldCurrentElement);
    FCurrentElement = Element;
    FStoredSessionsOpened = (MungedSubKey == FStoredSessionsSubKey);
  }
  return Result;
}

void TXmlStorage::DoCloseSubKey()
{
  if (!FKeyHistory.empty() && !FSubElements.empty())
  {
    FCurrentElement = FSubElements.back();
    FSubElements.pop_back();
  }
  else
  {
    FCurrentElement = nullptr;
  }
}

bool TXmlStorage::DoDeleteSubKey(const UnicodeString & SubKey)
{
  bool Result = false;
  tinyxml2::XMLElement * Element = FindElement(SubKey);
  if (Element != nullptr)
  {
    FCurrentElement->DeleteChild(Element);
    Result = true;
  }
  return Result;
}

void TXmlStorage::DoGetSubKeyNames(TStrings * Strings)
{
  for (tinyxml2::XMLElement * Element = FCurrentElement->FirstChildElement();
    Element != nullptr; Element = Element->NextSiblingElement())
  {
    UnicodeString Val = GetValue(Element);
    Strings->Add(PuttyUnMungeStr(Val));
  }
}

void TXmlStorage::DoGetValueNames(TStrings * /*Strings*/)
{
  ThrowNotImplemented(3022);
  // FRegistry->GetValueNames(Strings);
}

bool TXmlStorage::DoDeleteValue(const UnicodeString & Name)
{
  bool Result = false;
  tinyxml2::XMLElement * Element = FindElement(Name);
  if (Element != nullptr)
  {
    FCurrentElement->DeleteChild(Element);
    Result = true;
  }
  return Result;
}

void TXmlStorage::RemoveIfExists(const UnicodeString & Name)
{
  tinyxml2::XMLElement * Element = FindElement(Name);
  if (Element != nullptr)
  {
    FCurrentElement->DeleteChild(Element);
  }
}

void TXmlStorage::AddNewElement(const UnicodeString & Name, const UnicodeString & Value)
{
  const AnsiString StrName(Name);
  const AnsiString StrValue(Value);
  tinyxml2::XMLElement * Element = FXmlDoc->NewElement(StrName.c_str());
  Element->LinkEndChild(FXmlDoc->NewText(StrValue.c_str()));
  FCurrentElement->LinkEndChild(Element);
}

UnicodeString TXmlStorage::GetSubKeyText(const UnicodeString & Name) const
{
  const tinyxml2::XMLElement * Element = FindElement(Name);
  if (!Element)
  {
    return UnicodeString();
  }
  if (ToUnicodeString(CONST_SESSION_NODE) == Name)
  {
    return ToUnicodeString(Element->Attribute(CONST_NAME_ATTR));
  }
  return ToUnicodeString(Element->GetText());
}

tinyxml2::XMLElement * TXmlStorage::FindElement(const UnicodeString & Name) const
{
  for (const tinyxml2::XMLElement * Element = FCurrentElement->FirstChildElement();
    Element != nullptr; Element = Element->NextSiblingElement())
  {
    UnicodeString ElementName = ToUnicodeString(Element->Name());
    if (ElementName == Name)
    {
      return const_cast<tinyxml2::XMLElement *>(Element);
    }
  }
  return nullptr;
}

tinyxml2::XMLElement * TXmlStorage::FindChildElement(const AnsiString & SubKey) const
{
  tinyxml2::XMLElement * Result = nullptr;
  // DebugAssert(FCurrentElement);
  if (FStoredSessionsOpened)
  {
    tinyxml2::XMLElement * Element = FCurrentElement->FirstChildElement(CONST_SESSION_NODE);
    if (Element && !strcmp(Element->Attribute(CONST_NAME_ATTR), SubKey.c_str()))
    {
      Result = Element;
    }
  }
  else if (FCurrentElement)
  {
    Result = FCurrentElement->FirstChildElement(SubKey.c_str());
  }
  return Result;
}

UnicodeString TXmlStorage::GetValue(const tinyxml2::XMLElement * Element) const
{
  DebugAssert(Element);
  UnicodeString Result;
  if (FStoredSessionsOpened && Element->Attribute(CONST_NAME_ATTR))
  {
    Result = ToUnicodeString(Element->Attribute(CONST_NAME_ATTR));
  }
  else
  {
    Result = ToUnicodeString(Element->GetText());
  }
  return Result;
}

bool TXmlStorage::DoValueExists(const UnicodeString & Value, bool ForceAnsi)
{
  bool Result = false;
  const tinyxml2::XMLElement * Element = FindElement(Value);
  if (Element != nullptr)
  {
    Result = true;
  }
  return Result;
}

int32_t TXmlStorage::DoBinaryDataSize(const UnicodeString & /*Name*/)
{
  ThrowNotImplemented(3026);
  const int32_t Result = 0; // FRegistry->GetDataSize(Name);
  return Result;
}

UnicodeString TXmlStorage::GetSource() const
{
  return GetStorage();
}

UnicodeString TXmlStorage::GetSource()
{
  return GetStorage();
}

bool TXmlStorage::DoReadBool(const UnicodeString & Name, bool Default)
{
  const UnicodeString Result = ReadString(Name, L"");
  if (Result.IsEmpty())
  {
    return Default;
  }
  return AnsiCompareIC(Result, BooleanToEngStr(true)) == 0;
}

int32_t TXmlStorage::DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping)
{
  return ::StrToIntDef(GetSubKeyText(Name), Default);
}

int64_t TXmlStorage::DoReadInt64(const UnicodeString & Name, int64_t Default)
{
  return ::StrToInt64Def(GetSubKeyText(Name), Default);
}

TDateTime TXmlStorage::DoReadDateTime(const UnicodeString & Name, const TDateTime & Default)
{
  const double Result = ReadFloat(Name, Default.GetValue());
  return TDateTime(Result);
}

double TXmlStorage::DoReadFloat(const UnicodeString & Name, double Default)
{
  return ::StrToFloatDef(GetSubKeyText(Name), Default);
}

UnicodeString TXmlStorage::DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default)
{
  UnicodeString Result = GetSubKeyText(Name);
  return Result.IsEmpty() ? Default : Result;
}

int32_t TXmlStorage::DoReadBinaryData(const UnicodeString & /*Name*/,
  void * /*Buffer*/, int32_t /*Size*/)
{
  ThrowNotImplemented(3028);
  const int32_t Result = 0;
  return Result;
}

void TXmlStorage::DoWriteBool(const UnicodeString & Name, bool Value)
{
  WriteString(Name, ::BooleanToEngStr(Value));
}

void TXmlStorage::DoWriteInteger(const UnicodeString & Name, int32_t Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::IntToStr(Value));
}

void TXmlStorage::DoWriteInt64(const UnicodeString & Name, int64_t Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::Int64ToStr(Value));
}

/*void TXmlStorage::DoWriteFloat(const UnicodeString & Name, double Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, FORMAT("%.5f", Value));
}*/

void TXmlStorage::DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, Value);
}

void TXmlStorage::DoWriteBinaryData(const UnicodeString & Name,
  const uint8_t * Buffer, int32_t Size)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::StrToHex(UnicodeString(reinterpret_cast<const wchar_t *>(Buffer), Size), true));
}

int32_t TXmlStorage::GetFailed() const
{
  const int32_t Result = FFailed;
  FFailed = 0;
  return Result;
}
