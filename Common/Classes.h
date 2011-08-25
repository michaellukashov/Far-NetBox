#pragma once

#include <WinDef.h>

#pragma warning(push, 1)
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <assert.h>

#pragma warning(pop)

using namespace std;

//---------------------------------------------------------------------------
class TObject;
typedef void (TObject::*TThreadMethod)();
typedef void (TObject::*TNotifyEvent)(TObject *);
//---------------------------------------------------------------------------
class TObject
{
public:
    TObject() :
        FOwnsObjects(false)
    {}
    virtual ~TObject()
    {}

    virtual void Change()
    {}
    void OwnsObjects(bool value) { FOwnsObjects = value; }

private:
    bool FOwnsObjects;
};

struct TPoint
{
    int x;
    int y;
    TPoint() :
        x(0),
        y(0)
    {}
    TPoint(int x, int y) :
        x(x),
        y(y)
    {}
};

struct TRect
{
    int Left;
    int Top;
    int Right;
    int Bottom;
    int Width() const { return Right - Left; }
    int Height() const { return Bottom - Top; }
    TRect() :
        Left(0),
        Top(0),
        Right(0),
        Bottom(0)
    {}
    TRect(int left, int top, int right, int bottom) :
        Left(left),
        Top(top),
        Right(right),
        Bottom(bottom)
    {}
    bool operator == (const TRect &other)
    {
        return
            Left == other.Left &&
            Top == other.Top &&
            Right == other.Right &&
            Bottom == other.Bottom;
    }
    bool operator != (const TRect &other)
    {
        return !(operator == (other));
    }
    bool operator == (const RECT &other)
    {
        return
            Left == other.left &&
            Top == other.top &&
            Right == other.right &&
            Bottom == other.bottom;
    }
    bool operator != (const RECT &other)
    {
        return !(operator == (other));
    }
};

class TPersistent : public TObject
{
public:
    virtual void Assign(TPersistent *Source)
    {}
};

class TObjectList : public TPersistent
{
public:
    size_t GetCount() const { return m_objects.size(); }

    TObject * operator [](size_t Index) const
    {
        return m_objects[Index];
    }
    TObject * GetItem(size_t Index) const
    {
        return m_objects[Index];
    }

    size_t Add(TObject *value)
    {
        m_objects.push_back(value);
        return m_objects.size() - 1;
    }
    void Remove(TObject *value)
    {
    }
    size_t IndexOf(TObject *value) const
    {
        return -1;
    }
    bool GetOwnsObjects() { return FOwnsObjects; }
    void SetOwnsObjects(bool value) { FOwnsObjects = value; }

private:
    vector<TObject *> m_objects;
    bool FOwnsObjects;
};

class TList : public TObjectList
{
public:
};

class TStrings : public TPersistent
{
public:
    size_t Add(std::wstring value)
    {
        return -1;
    }
    virtual size_t GetCount()
    {
        return 0;
    }
    std::wstring GetString(int Index)
    {
        return L"";
    }
    std::wstring GetText()
    {
        return L"";
    }
    void SetText(std::wstring S)
    {
    }
    void *GetObject(int Index)
    {
        return NULL;
    }
    void SetObject(int Index, TObject *obj)
    {
    }
    void AddObject(std::wstring str, TObject *obj)
    {
    }

    TNotifyEvent GetOnChange() { return FOnChange; }
    void SetOnChange(TNotifyEvent Event) { FOnChange = Event; }
    bool Equals(TStrings *value)
    {
        return false;
    }
    virtual void Clear()
    {
    }
    virtual void PutObject(int Index, TObject *AObject)
    {
    }
private:
    TNotifyEvent FOnChange;
};

class TStringList : public TStrings
{
public:
    virtual void Assign(TPersistent *Source)
    {}
    void Put(int Index, std::wstring value)
    {
    }
    int GetUpdateCount()
    {
        return 0;
    }
    void Changed()
    {
    }
    size_t IndexOf(const wchar_t *value)
    {
        return -1;
    }
    void Delete(size_t Index)
    {
    }
    void SetString(int Index, std::wstring S)
    {
    }
	void SetCaseSensitive(bool value)
	{
	}
};

class TDateTime
{
public:
    operator double()
    {
        return 0.0;
    }
};

static TDateTime Now()
{
    TDateTime result;
    return result;
}

//---------------------------------------------------------------------------

