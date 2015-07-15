#include "test_dll.h"

#include <string>

namespace {

void define_test_namespace(rtti::meta_define<void> define)
{
    define
        ._attribute("Description", "Rtti test namespace")
        ._enum<test::Color>("Color")
            ._element("Red", test::Color::Red)
            ._element("Green", test::Color::Green)
            ._element("Blue", test::Color::Blue)

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

        ._class<test::TestBase1>("TestBase1")
            ._enum<test::TestBase1::TestEnum>("TestEnum")
                ._element("te1", test::TestBase1::te1)
                ._element("te2", test::TestBase1::te2)
        ._end()

        ._class<test::TestBase2>("TestBase2")
        ._end()

        ._class<test::TestDerived>("TestDerived")
            ._base<test::TestBase1, test::TestBase2>()
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
        ._attribute("Description", "Standard C++ library")
        ._class<std::string>("string")
            ._lazy(define_std_string)
        ._end()
    ;
}

}

void register_rtti()
{
    rtti::global_define()
        ._attribute("Description", "Global namespace")
        ._attribute("Attribute 1", std::string{"standard string object"})
        ._attribute("Attribute 2", true)
        ._attribute("Attribute 3", 3.14)

        ._namespace("test")
            ._lazy(define_test_namespace)
        ._end()

        ._namespace("std")
            ._lazy(define_std_namespace)
        ._end();
    ;

    register_std_vector<int>();
    register_std_vector<std::string>();
    register_std_vector<const char*>();
}

