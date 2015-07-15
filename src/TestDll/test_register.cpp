#include "test_dll.h"

#include <string>

namespace {

void define_test_namespace(rtti::meta_define<void> define)
{
    define
        ._class<test::Absolute>("Absolute")
            ._attribute("Description", "Absolutly empty class: Absolute{}")
        ._end()
        ._class<test::Empty>("Empty")
            ._attribute("Description", "Class without any fields")
            ._attribute("Polymorphic", false)
        ._end()
        ._class<test::Small>("Small")
            ._attribute("Polymorphic", true)
            ._constructor<std::int8_t>()
        ._end()
    ;
}

void define_std_string(rtti::meta_define<std::string> define)
{
    define
        ._constructor<const char*>()
    ;
}

void define_std_namespace(rtti::meta_define<void> define)
{
    define
        ._class<std::string>("string")
            ._lazy(define_std_string)
        ._end()
    ;
}

}

void register_rtti()
{
    rtti::global_define()
        ._namespace("test")
            ._lazy(define_test_namespace)
        ._end()
        ._namespace("std")
            ._lazy(define_std_namespace)
        ._end();
    ;
}

