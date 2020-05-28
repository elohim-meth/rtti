#include <rtti/metadefine.h>

std::string g_string = "";
std::string const gro_string = "Hello, World!";

std::string intToStr(int value, bool &ok)
{
    ok = true;
    return std::to_string(value);
}

enum class operation { add, subtract, multiply, divide };


void register_rtti()
{
    using namespace std::literals;

    rtti::global_define()
        ._attribute("Description", "Global namespace")
        ._attribute("string_attr", "Hello, World!"s)
        ._attribute("bool_attr", true)
        ._attribute("double_attr", 3.14)
        ._attribute("int_attr", 256)
        ._attribute("enum_attr", operation::multiply)
        ._property("g_string", &g_string)
        ._property("gro_string", &gro_string)
        ._method("intToStr", &intToStr)
        ._enum<operation>("operation")
            ._element("add", operation::add)
            ._element("subtract", operation::subtract)
            ._element("multiply", operation::multiply)
            ._element("divide", operation::divide)
   ;

    // default convert
    rtti::MetaType::registerConverter<char*, std::string>();
    rtti::MetaType::registerConverter<char*, std::string_view>();

    rtti::MetaType::registerConverter<bool, char>();

    rtti::MetaType::registerConverter<short, int>();
    rtti::MetaType::registerConverter<unsigned short, int>();
    rtti::MetaType::registerConverter<int, unsigned int>();
    rtti::MetaType::registerConverter<unsigned int, int>();
    rtti::MetaType::registerConverter<int, long int>();
    rtti::MetaType::registerConverter<long int, int>();
    rtti::MetaType::registerConverter<int, long long int>();
    rtti::MetaType::registerConverter<int, unsigned long long int>();
    rtti::MetaType::registerConverter<float, double>();
    rtti::MetaType::registerConverter<float, long double>();
    rtti::MetaType::registerConverter<double, long double>();
}

