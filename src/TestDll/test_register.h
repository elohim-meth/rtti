#ifndef TEST_REGISTER_H
#define TEST_REGISTER_H

#include <rtti.h>

#include <vector>

template<typename T>
void define_std_vector(rtti::meta_define<std::vector<T>> define)
{
    using V = std::vector<T>;
    define
        .template _constructor<typename V::size_type>()
        .template _constructor<typename V::size_type, const typename V::value_type&>()
        .template _constructor<std::initializer_list<typename V::value_type>>()
        .template _constructor<typename V::iterator, typename V::iterator>()
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

void register_rtti();

#endif // TEST_REGISTER_H

