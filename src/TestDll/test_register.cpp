#include "test_dll.h"

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

}

void register_test_namespace()
{
    rtti::global_define()
        ._namespace("test")
            ._lazy(define_test_namespace)
        ._end();
    ;
}

