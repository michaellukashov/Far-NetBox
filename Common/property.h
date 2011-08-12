#pragma once

template <class object, class type>
class property
{
public:

    typedef type (object::*get_proc)() const;
    typedef void (object::*set_proc)(type const &);

    property(object &object, get_proc getpr, set_proc setpr) :
        obj(object)
        , getter(getpr)
        , setter(setpr)
    {
    }

    operator type()
    {
        return get();
    }

    type const &operator = (type const &value)
    {
        set(value);
        return value;
    }

    property &operator = (property const &prt)
    {
        set(prt.get());
        return *this;
    }

private:
    object &obj;
    get_proc getter;
    set_proc setter;

    inline type get() const
    {
        return (obj.*getter)();
    }

    inline void set(type const &value)
    {
        (obj.*setter)(value);
    }
};

