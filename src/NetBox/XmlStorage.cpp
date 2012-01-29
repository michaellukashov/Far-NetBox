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
TXmlStorage::TXmlStorage(const std::wstring AStorage,
    const std::wstring StoredSessionsSubKey) :
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
}
//---------------------------------------------------------------------------
TXmlStorage::~TXmlStorage()
{
    if (GetAccessMode() == smReadWrite)
    {
        WriteXml();
    }
    delete FXmlDoc;
};
//---------------------------------------------------------------------------
bool TXmlStorage::LoadXml()
{
    CNBFile xmlFile;
    if (!xmlFile.OpenRead(GetStorage().c_str()))
    {
        return false;
    }
    size_t buffSize = static_cast<size_t>(xmlFile.GetFileSize() + 1);
    if (buffSize > 1000000)
    {
        return false;
    }
    std::string buff(buffSize, 0);
    if (!xmlFile.Read(&buff[0], buffSize))
    {
        return false;
    }

    FXmlDoc->Parse(buff.c_str());
    if (FXmlDoc->Error())
    {
        return false;
    }

    // Get and check root node
    TiXmlElement *xmlRoot = FXmlDoc->RootElement();
    if (strcmp(xmlRoot->Value(), CONST_ROOT_NODE) != 0)
    {
        return false;
    }
    if (strcmp(xmlRoot->Attribute(CONST_VERSION_ATTR), CONST_XML_VERSION) != 0)
    {
        return false;
    }
    TiXmlElement *Element = xmlRoot->FirstChildElement(ToStdString(FStoredSessionsSubKey).c_str());
    if (Element != NULL)
    {
        FCurrentElement = FXmlDoc->RootElement();
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
bool TXmlStorage::WriteXml()
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
bool TXmlStorage::Copy(TXmlStorage *Storage)
{
    nb::Error(SNotImplemented, 3020);
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
    switch (GetAccessMode())
    {
    case smRead:
        LoadXml();
        break;

    case smReadWrite:
    default:
        FXmlDoc->LinkEndChild(new TiXmlDeclaration("1.0", "UTF-8", ""));
        assert(FCurrentElement == NULL);
        FCurrentElement = new TiXmlElement(CONST_ROOT_NODE);
        FCurrentElement->SetAttribute(CONST_VERSION_ATTR, CONST_XML_VERSION);
        FXmlDoc->LinkEndChild(FCurrentElement);
        break;
    }
}
//---------------------------------------------------------------------------
bool TXmlStorage::OpenSubKey(const std::wstring SubKey, bool CanCreate, bool Path)
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
        std::string subKey = ToStdString(PuttyMungeStr(SubKey));
        if (CanCreate)
        {
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
            Element = FindChildElement(subKey);
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
bool TXmlStorage::DeleteSubKey(const std::wstring SubKey)
{
    bool result = false;
    TiXmlElement *Element = FindElement(SubKey);
    if (Element != NULL)
    {
        FCurrentElement->RemoveChild(Element);
        result = true;
    }
    return result;
}
//---------------------------------------------------------------------------
void TXmlStorage::GetSubKeyNames(nb::TStrings *Strings)
{
    for (TiXmlElement *Element = FCurrentElement->FirstChildElement();
            Element != NULL; Element = Element->NextSiblingElement())
    {
        std::wstring val = GetValue(Element);
        Strings->Add(PuttyUnMungeStr(val));
    }
}
//---------------------------------------------------------------------------
void TXmlStorage::GetValueNames(nb::TStrings *Strings)
{
    nb::Error(SNotImplemented, 3022);
    // FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TXmlStorage::DeleteValue(const std::wstring Name)
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
bool TXmlStorage::KeyExists(const std::wstring SubKey)
{
    nb::Error(SNotImplemented, 3024);
    std::wstring K = PuttyMungeStr(SubKey);
    bool Result = false; // FRegistry->KeyExists(K);
    return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::RemoveIfExists(const std::wstring Name)
{
    TiXmlElement *Element = FindElement(Name);
    if (Element != NULL)
    {
        FCurrentElement->RemoveChild(Element);
    }
}
//---------------------------------------------------------------------------
void TXmlStorage::AddNewElement(const std::wstring Name, const std::wstring Value)
{
    std::string name = ToStdString(Name);
    std::string value = ToStdString(Value);
    TiXmlElement *Element = new TiXmlElement(name);
    Element->LinkEndChild(new TiXmlText(value.c_str()));
    FCurrentElement->LinkEndChild(Element);
}
//---------------------------------------------------------------------------
std::wstring TXmlStorage::GetSubKeyText(const std::wstring Name)
{
    TiXmlElement *Element = FindElement(Name);
    if (!Element)
    {
        return std::wstring();
    }
    return ToStdWString(Element->GetText());
}
//---------------------------------------------------------------------------
TiXmlElement *TXmlStorage::FindElement(const std::wstring Name)
{
    for (const TiXmlElement *Element = FCurrentElement->FirstChildElement();
            Element != NULL; Element = Element->NextSiblingElement())
    {
        std::wstring name = ToStdWString(Element->ValueStr());
        // DEBUG_PRINTF(L"name = %s", name.c_str());
        if (name == Name)
        {
            return const_cast<TiXmlElement *>(Element);
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
TiXmlElement *TXmlStorage::FindChildElement(const std::string &subKey)
{
    TiXmlElement *result = NULL;
    assert(FCurrentElement);
    if (FStoredSessionsOpened)
    {
        TiXmlElement *Element = FCurrentElement->FirstChildElement(CONST_SESSION_NODE);
        if (Element && !strcmp(Element->Attribute(CONST_NAME_ATTR), subKey.c_str()))
        {
            result = Element;
        }
    }
    else
    {
        result = FCurrentElement->FirstChildElement(subKey.c_str());
    }
    return result;
}
//---------------------------------------------------------------------------
std::wstring TXmlStorage::GetValue(TiXmlElement *Element)
{
    assert(Element);
    std::wstring result;
    if (FStoredSessionsOpened && Element->Attribute(CONST_NAME_ATTR))
    {
        result = ToStdWString(Element->Attribute(CONST_NAME_ATTR));
    }
    else
    {
        result = ToStdWString(Element->ValueStr());
    }
    return result;
}
//---------------------------------------------------------------------------
bool TXmlStorage::ValueExists(const std::wstring Value)
{
    // nb::Error(SNotImplemented, 3025);
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
int TXmlStorage::BinaryDataSize(const std::wstring Name)
{
    nb::Error(SNotImplemented, 3026);
    int Result = 0; // FRegistry->GetDataSize(Name);
    return Result;
}
//---------------------------------------------------------------------------
bool TXmlStorage::Readbool(const std::wstring Name, bool Default)
{
    std::wstring res = ReadString(Name, L"");
    if (res.empty())
    {
        return Default;
    }
    else
    {
        return AnsiCompareIC(res, ::BooleanToEngStr(true)) == 0;
    }
}
//---------------------------------------------------------------------------
nb::TDateTime TXmlStorage::ReadDateTime(const std::wstring Name, nb::TDateTime Default)
{
    double res = ReadFloat(Name, Default.operator double());
    return nb::TDateTime(res);
}
//---------------------------------------------------------------------------
double TXmlStorage::ReadFloat(const std::wstring Name, double Default)
{
    return StrToFloatDef(GetSubKeyText(Name), Default);
}
//---------------------------------------------------------------------------
int TXmlStorage::Readint(const std::wstring Name, int Default)
{
    return StrToIntDef(GetSubKeyText(Name), Default);
}
//---------------------------------------------------------------------------
__int64 TXmlStorage::ReadInt64(const std::wstring Name, __int64 Default)
{
    return StrToInt64Def(GetSubKeyText(Name), Default);
}
//---------------------------------------------------------------------------
std::wstring TXmlStorage::ReadStringRaw(const std::wstring Name, const std::wstring Default)
{
    std::wstring result = GetSubKeyText(Name);
    return result.empty() ? Default : result;
}
//---------------------------------------------------------------------------
int TXmlStorage::ReadBinaryData(const std::wstring Name,
                                void *Buffer, size_t Size)
{
    nb::Error(SNotImplemented, 3028);
    int Result = 0;
    return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::Writebool(const std::wstring Name, bool Value)
{
    WriteString(Name, ::BooleanToEngStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteDateTime(const std::wstring Name, nb::TDateTime Value)
{
    WriteFloat(Name, Value);
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteFloat(const std::wstring Name, double Value)
{
    RemoveIfExists(Name);
    AddNewElement(Name, FORMAT(L"%.5f", Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteStringRaw(const std::wstring Name, const std::wstring Value)
{
    RemoveIfExists(Name);
    AddNewElement(Name, Value);
}
//---------------------------------------------------------------------------
void TXmlStorage::Writeint(const std::wstring Name, int Value)
{
    RemoveIfExists(Name);
    AddNewElement(Name, ::IntToStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteInt64(const std::wstring Name, __int64 Value)
{
    RemoveIfExists(Name);
    AddNewElement(Name, ::Int64ToStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteBinaryData(const std::wstring Name,
                                  const void *Buffer, size_t Size)
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
