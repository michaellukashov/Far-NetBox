#pragma once

template <class object, class type>
class property_ro
{
public:

    typedef type(object::*get_proc)() const;

    property_ro() :
        obj(NULL)
        , getter(NULL)
    {
    }
    property_ro(object *object, get_proc getpr) :
        obj(object)
        , getter(getpr)
    {
    }

    operator type()
    {
        return get();
    }

    property_ro &operator = (property_ro const &prt)
    {
        obj = prt.obj;
        getter = prt.getter;
        return *this;
    }

private:
    object *obj;
    get_proc getter;

    inline type get() const
    {
        return ((*obj).*getter)();
    }
};

template <class object, class type>
class property_idx
{
public:

    typedef type(object::*get_proc)(int) const;

    property_idx() :
        obj(NULL)
        , getter(NULL)
    {
    }
    property_idx(object *object, get_proc getpr) :
        obj(object)
        , getter(getpr)
    {
    }

    type operator [](int Index) const
    {
        return get(Index);
    }

    property_idx &operator = (property_idx const &prt)
    {
        obj = prt.obj;
        getter = prt.getter;
        return *this;
    }

private:
    object *obj;
    get_proc getter;

    inline type get(int Index) const
    {
        return ((*obj).*getter)(Index);
    }
};

template <class object, class type>
class property
{
public:

    typedef type(object::*get_proc)() const;
    typedef void(object::*set_proc)(const type &);

    property() :
        obj(NULL)
        , getter(NULL)
        , setter(NULL)
    {
    }
    property(object *object, get_proc getpr, set_proc setpr) :
        obj(object)
        , getter(getpr)
        , setter(setpr)
    {
    }

    operator type()
    {
        return get();
    }

    const type &operator = (const type &value)
    {
        set(value);
        return value;
    }

    property &operator = (const property &prt)
    {
        set(prt.get());
        return *this;
    }

    inline bool operator == (const type &value)
    {
        return get() == value;
    }

    inline bool operator != (const type &value)
    {
        return !(operator == (value));
    }

private:
    object *obj;
    get_proc getter;
    set_proc setter;

    inline type get() const
    {
        return ((*obj).*getter)();
    }

    inline void set(const type &value)
    {
        ((*obj).*setter)(value);
    }
};

