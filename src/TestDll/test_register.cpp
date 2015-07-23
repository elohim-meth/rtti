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
        ._constructor<const char*, std::size_t>()
    ;
}

struct define_std_namespace
{
    void operator()(rtti::meta_define<void> define)
    {
        define
            ._attribute("Description", "Standard C++ library")
            ._class<std::string>("string")
                ._lazy(define_std_string)
            ._end()
        ;
    }
};

template<typename From>
inline bool register_toString_converter()
{
    return rtti::MetaType::registerConverter(
                static_cast<std::string(*)(From)>(&std::to_string));
}

template<typename From>
bool register_asString_converter()
{
    auto lambda = [](From from) -> std::string
    {
        std::ostringstream os;
        os << std::boolalpha << from;
        return os.str();

    };
    return rtti::MetaType::registerConverter<From, std::string>(lambda);
}

template<typename To>
bool register_fromString_converter()
{
    auto lambda = [](const std::string &from) -> To
    {
        std::istringstream is{from};
        To to;
        is >> std::boolalpha >> to;
        return std::move(to);

    };
    return rtti::MetaType::registerConverter<std::string, To>(lambda);
}

void test_method(const char *)
{ PRINT_PRETTY_FUNC }

void test_method(int)
{ PRINT_PRETTY_FUNC }

test::Small test_return(std::int8_t value)
{
    return test::Small(value);
}

}

void register_rtti()
{
    rtti::global_define()
        ._attribute("Description", "Global namespace")
        ._attribute("Attribute 1", std::string{"standard string object"})
        ._attribute("Attribute 2", true)
        ._attribute("Attribute 3", 3.14)
        ._method<void(const char*)>("test_method", &test_method)
        ._method<void(int)>("test_method", &test_method)
        ._method("test_return", &test_return)

        ._namespace("test")
            ._lazy(define_test_namespace)
        ._end()

        ._namespace("std")
            ._lazy(define_std_namespace{})
        ._end();
    ;

    register_std_vector<int>();
    register_std_vector<std::string>();
    register_std_vector<const char*>();

    // default
    rtti::MetaType::registerConverter<char*, std::string>();

    // std::to_string
    register_toString_converter<int>();
    register_toString_converter<unsigned>();
    register_toString_converter<long>();
    register_toString_converter<unsigned long>();
    register_toString_converter<long long>();
    register_toString_converter<unsigned long long>();
    register_toString_converter<float>();
    register_toString_converter<double>();
    register_toString_converter<long double>();

    // std::ostringstream
    register_asString_converter<bool>();

    rtti::MetaType::registerConverter<std::string, int>(
                [] (const std::string &value)
                {
                    return std::stoi(value);
                });

    register_fromString_converter<bool>();

}

