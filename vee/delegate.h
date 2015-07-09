#ifndef _VEE_DELEGATE_H_
#define _VEE_DELEGATE_H_

#include <cassert>
#include <map>
#include <vector>
#include <functional>
#include <vee/macro.h>

_VEE_BEGIN

template <class T>
void* pvoid_cast(T pointer)
{
    auto& ptr = pointer;
    void* addr = *(void**)(&ptr);
    return addr;
}

template <typename FuncSig, typename _Function>
bool compare_functions(const _Function& lhs, const _Function& rhs)
{
    typedef typename std::conditional
        <
        std::is_function<FuncSig>::value,
        typename std::add_pointer<FuncSig>::type,
        FuncSig
        > ::type target_type;
    if (const target_type* lhs_internal = lhs.template target<target_type>())
    {
        if (const target_type* rhs_internal = rhs.template target<target_type>())
            return *rhs_internal == *lhs_internal;
    }
    return false;
}

template <typename FTy>
class compareable_function: std::function < FTy >
{
    typedef std::function<FTy> _function_t;
    bool(*_type_holder)(const _function_t&, const _function_t&);
public:
    compareable_function() = default;
    template <typename Func> compareable_function(Func f):
        _function_t(f),
        _type_holder(compare_functions<Func, _function_t>)
    {
        // empty
    }
    template <typename Func> compareable_function& operator=(Func f)
    {
        _function_t::operator =(f);
        _type_holder = compare_functions < Func, _function_t > ;
        return *this;
    }
    friend bool operator==(const _function_t& lhs, const compareable_function& rhs)
    {
        return rhs._type_holder(lhs, rhs);
    }
    friend bool operator==(const compareable_function &lhs, const _function_t &rhs)
    {
        return rhs == lhs;
    }
};

template <class T>
class delegate
{
};

template <class RTy, class ...Args>
class delegate < RTy(Args ...) >
{
public:
    typedef delegate< RTy(Args ...) > type;
    typedef type& reference_t;
    typedef type&& rreference_t;
    typedef RTy(*funcptr_t)(Args ...);
    typedef __int64 key_t;
private:
    typedef std::function< RTy(Args ...) > _binder_t;
    typedef compareable_function< RTy(Args ...) > _compareable_binder_t;
    typedef std::vector< _binder_t > _container_t;
    typedef std::map<key_t, _binder_t> _indexing_container_t;
public:
    // ctor
    delegate() = default;
    // dtor
    ~delegate() = default;
    // copy ctor
    delegate(reference_t _left)
    {
        this->_deep_copy(_left);
    }
    // move ctor
    delegate(rreference_t _right)
    {
        this->_deep_copy(_right);
    }
    void operator()(Args&& ...args)
    {
        for (auto& it : _container)
        {
            it.operator()(std::forward<decltype(args)>(args)...);
        }
        for (auto& it : _idx_container)
        {
            it.second.operator()(std::forward<decltype(args)>(args)...);
        }
    }
    template <class CallableObj>
    void operator+=(CallableObj func)
    {
        _binder_t binder(func);
        _container.push_back(std::move(binder));
    }
    template <class CallableObj>
    void operator+=(std::pair<key_t, CallableObj> key_func_pair)
    {
        _binder_t binder(key_func_pair.second);
        _idx_container.insert(std::make_pair(key_func_pair.first, binder));
    }
    template <class CallableObj>
    bool operator-=(CallableObj func)
    {
        _binder_t binder(func);
        for (auto iter = _container.begin(); iter != _container.end(); ++iter)
        {
            if (compare_functions<CallableObj>(*iter, binder))
            {
                _container.erase(iter);
                return true;
            }
        }
        printf("REMOVE FAILURE (function not found)\n");
        return false;
    }
    bool operator-=(const key_t key)
    {
        for (auto iter = _idx_container.begin(); iter != _idx_container.end(); ++iter)
        {
            if (iter->first == key)
            {
                _idx_container.erase(iter);
                return true;
            }
        }
        printf("REMOVE FAILURE (key not found)\n");
        return false;
    }
private:
    void _deep_copy(reference_t _left)
    {
        _container = _left._container;
    }
    void _deep_copy(rreference_t _right)
    {
        _container = std::move(_right._container);
    }
private:
    _container_t _container;
    _indexing_container_t _idx_container;
};

_VEE_END

#endif // _VEE_DELEGATE_H_