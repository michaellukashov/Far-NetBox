#define COMBINE(a,b)           COMBINE1(a,b)
#define COMBINE1(a,b)          a##b

#define I_DELEGATE             COMBINE(IDelegate, SUFFIX)
#define C_STATIC_DELEGATE      COMBINE(CStaticDelegate, SUFFIX)
#define C_METHOD_DELEGATE      COMBINE(CMethodDelegate, SUFFIX)
#define C_DELEGATE             COMBINE(CDelegate, SUFFIX)

template<class TRet TEMPLATE_PARAMS>
class I_DELEGATE
{
public:
    virtual ~I_DELEGATE() {}
    virtual TRet Invoke(PARAMS) = 0;
    virtual bool Compare(I_DELEGATE<TRet TEMPLATE_ARGS>* pDelegate) = 0;
};


template<class TRet TEMPLATE_PARAMS>
class C_STATIC_DELEGATE : public I_DELEGATE<TRet TEMPLATE_ARGS>
{
public:
    typedef TRet (*PFunc)(PARAMS);
    C_STATIC_DELEGATE(PFunc pFunc) { m_pFunc = pFunc; }
    virtual TRet Invoke(PARAMS) { return m_pFunc(ARGS); }
    virtual bool Compare(I_DELEGATE<TRet TEMPLATE_ARGS>* pDelegate)
    {
        C_STATIC_DELEGATE<TRet TEMPLATE_ARGS>* pStaticDel =
            dynamic_cast<C_STATIC_DELEGATE<TRet TEMPLATE_ARGS>*>(pDelegate);
        if(pStaticDel == NULL || pStaticDel->m_pFunc != m_pFunc)
            return false;

        return true;
    }

private:
    PFunc m_pFunc;
};


template<class TObj, class TRet TEMPLATE_PARAMS>
class C_METHOD_DELEGATE : public I_DELEGATE<TRet TEMPLATE_ARGS>
{
public:
    typedef TRet (TObj::*PMethod)(PARAMS);
    C_METHOD_DELEGATE(TObj* pObj, PMethod pMethod)
    {
        m_pObj = pObj;
        m_pMethod = pMethod;
    }
    virtual TRet Invoke(PARAMS) { return (m_pObj->*m_pMethod)(ARGS); }
    virtual bool Compare(I_DELEGATE<TRet TEMPLATE_ARGS>* pDelegate)
    {
        C_METHOD_DELEGATE<TObj, TRet TEMPLATE_ARGS>* pMethodDel =
            dynamic_cast<C_METHOD_DELEGATE<TObj, TRet TEMPLATE_ARGS>* >(pDelegate);
        if
        (
            pMethodDel == NULL ||
            pMethodDel->m_pObj != m_pObj ||
            pMethodDel->m_pMethod != m_pMethod
        )
        {
            return false;
        }

        return true;
    }

private:
    TObj *m_pObj;
    PMethod m_pMethod;
};


template<class TRet TEMPLATE_PARAMS>
I_DELEGATE<TRet TEMPLATE_ARGS>* NewDelegate(TRet (*pFunc)(PARAMS))
{
    return new C_STATIC_DELEGATE<TRet TEMPLATE_ARGS>(pFunc);
}

template <class TObj, class TRet TEMPLATE_PARAMS>
I_DELEGATE<TRet TEMPLATE_ARGS>* NewDelegate(TObj* pObj, TRet (TObj::*pMethod)(PARAMS))
{
    return new C_METHOD_DELEGATE<TObj, TRet TEMPLATE_ARGS> (pObj, pMethod);
}


template<class TRet TEMPLATE_PARAMS>
class C_DELEGATE
{
public:
    typedef I_DELEGATE<TRet TEMPLATE_ARGS> IDelegate;
    typedef std::list<IDelegate*> DelegateList;

    C_DELEGATE(IDelegate* pDelegate = NULL) { Add(pDelegate); }
    ~C_DELEGATE() { RemoveAll(); }
    bool IsNull() { return (m_DelegateList.empty()); }

    C_DELEGATE<TRet TEMPLATE_ARGS>& operator=(IDelegate* pDelegate)
    {
        RemoveAll();
        Add(pDelegate);
        return *this;
    }
    
    C_DELEGATE<TRet TEMPLATE_ARGS>& operator+=(IDelegate* pDelegate)
    {
        Add(pDelegate);
        return *this;
    }

    C_DELEGATE<TRet TEMPLATE_ARGS>& operator-=(IDelegate* pDelegate)
    {
        Remove(pDelegate);
        return *this;
    }

    TRet operator()(PARAMS)
    {
        return Invoke(ARGS);
    }

private:
    void Add(IDelegate* pDelegate)
    {
        if(pDelegate != NULL)
            m_DelegateList.push_back(pDelegate);
    }

    void Remove(IDelegate* pDelegate)
    {
        DelegateList::iterator it;
        for(it = m_DelegateList.begin(); it != m_DelegateList.end(); ++it)
        {
            if((*it)->Compare(pDelegate))
            {
                delete (*it);
                m_DelegateList.erase(it);
                break;
            }
        }
    }

    void RemoveAll()
    {
        DelegateList::iterator it;
        for(it = m_DelegateList.begin(); it != m_DelegateList.end(); ++it)
            delete (*it);

        m_DelegateList.clear();
    }

    TRet Invoke(PARAMS)
    {
        DelegateList::const_iterator it;
        for(it = m_DelegateList.begin(); it != --m_DelegateList.end(); ++it)
            (*it)->Invoke(ARGS);

        return m_DelegateList.back()->Invoke(ARGS);
    }

private:
    DelegateList m_DelegateList;
};

#undef COMBINE
#undef COMBINE1

#undef I_DELEGATE
#undef C_STATIC_DELEGATE
#undef C_METHOD_DELEGATE
#undef C_DELEGATE