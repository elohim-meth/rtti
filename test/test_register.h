#ifndef TEST_REGISTER_H
#define TEST_REGISTER_H

#include <rtti/rtti.h>

#include <vector>
#include <memory>

template<typename T>
void define_std_vector(rtti::meta_define<std::vector<T>> define)
{
    using V = std::vector<T>;
    define
        .template _constructor<typename V::size_type>()
        .template _constructor<typename V::size_type, const typename V::value_type&>()
        .template _constructor<std::initializer_list<typename V::value_type>>()
        .template _constructor<typename V::pointer, typename V::pointer>("range1")
        .template _constructor<typename V::iterator, typename V::iterator>("range2")
    ;
}

template<typename T>
void define_std_unique_ptr(rtti::meta_define<std::unique_ptr<T>> define)
{
    using U = std::unique_ptr<T>;
    define
        .template _constructor<typename U::pointer>()
        .template _method("get", &U::get)
        .template _method("release", &U::release)
        .template _method("reset", &U::reset)
    ;
}


template<typename T>
void register_std_vector()
{
    rtti::global_define()
        ._namespace("std")
            ._class<std::vector<T>>("vector<" + type_name<T>() + ">")
                ._lazy(define_std_vector<T>)
            ._end()
        ._end()
    ;
}

template<typename T>
void register_std_unique_ptr()
{
    rtti::global_define()
        ._namespace("std")
            ._class<std::unique_ptr<T>>("unique_ptr<" + type_name<T>() + ">")
                ._lazy(define_std_unique_ptr<T>)
            ._end()
        ._end()
    ;
}

void register_rtti();

#endif // TEST_REGISTER_H

