#ifndef _ANY_H
#define _ANY_H

#include <memory>
#include <typeindex>
#include <cassert>
#include <common/check.h>

template<typename T>                                                            
struct identity { typedef T type; };  

struct Any
{
    Any(void) : m_tpIndex(std::type_index(typeid(void))){}
    Any(const Any& that) : m_ptr(that.Clone()), m_tpIndex(that.m_tpIndex) {}
    Any(Any && that) : m_ptr(std::move(that.m_ptr)), m_tpIndex(that.m_tpIndex) {}


    template<typename U>
    static Any from(typename identity<U>::type &&value)
    {
        return Any(value);
    }

    template<typename U>
    static Any from(const typename identity<U>::type &value)
    {
        return Any(value);
    }

    bool IsNull() const { return !bool(m_ptr); }

    template<class U> bool Is() const
    {
        return m_tpIndex == std::type_index(typeid(U));
    }

    template<class U>
    U& AnyCast()
    {
        CHECK(Is<U>(), "Cannot cast %s to %s", typeid(U).name(), m_tpIndex.name());
        // assert(Is<U>());

        auto derived = dynamic_cast<Derived<U>*> (m_ptr.get());
        return derived->m_value;
    }

    Any& operator=(const Any& a)
    {
        if (m_ptr == a.m_ptr)
            return *this;

        m_ptr = a.Clone();
        m_tpIndex = a.m_tpIndex;
        return *this;
    }

private:
    template<typename U, 
    class = typename std::enable_if<!std::is_same<typename std::decay<U>::type, Any>::value, U>::type> 
    Any(U && value) : m_ptr(new Derived < typename std::decay<U>::type>(std::forward<U>(value))),
        m_tpIndex(std::type_index(typeid(typename std::decay<U>::type))){}

    template<typename U, 
    class = typename std::enable_if<!std::is_same<typename std::decay<U>::type, Any>::value, U>::type> 
    Any(const U &value) : m_ptr(new Derived < typename std::decay<U>::type>(value)),
    m_tpIndex(std::type_index(typeid(typename std::decay<U>::type))){}

    struct Base;
    typedef std::unique_ptr<Base> BasePtr;

    struct Base
    {
        virtual ~Base() {}
        virtual BasePtr Clone() const = 0;
    };

    template<typename T>
    struct Derived : Base
    {
        template<typename U>
        Derived(U && value) : m_value(std::forward<U>(value)) { }

        template<typename U>
        Derived(const U & value) : m_value(value) { }

        BasePtr Clone() const
        {
            return BasePtr(new Derived<T>(m_value));
        }

        T m_value;
    };

    BasePtr Clone() const
    {
        if (m_ptr != nullptr)
            return m_ptr->Clone();

        return nullptr;
    }

    BasePtr m_ptr;
    std::type_index m_tpIndex;
};

#endif
