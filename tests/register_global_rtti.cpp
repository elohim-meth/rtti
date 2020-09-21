#include <rtti/metadefine.h>

std::string g_string = "";
std::string const gro_string = "Hello, World!";

std::string intToStr(int value, bool &ok)
{
    std::ostringstream os;
    os << std::boolalpha << value;
    ok = !os.fail();
    return os.str();
}

int strToInt(std::string const &value, bool &ok)
{
    std::istringstream is{value};
    int to;
    is >> std::boolalpha >> to;
    ok = !is.fail();
    return to;
}

template<typename From>
bool register_toString_converter()
{
    auto lambda = [](From from) -> std::string
    {
        std::ostringstream os;
        os.exceptions(std::ios::badbit | std::ios::failbit);
        os << std::boolalpha << from;
        return os.str();

    };
    return rtti::MetaType::registerConverter<From, std::string>(lambda);
}

template<typename To>
bool register_fromString_converter()
{
    auto lambda = [](std::string const &from, bool &ok) -> To
    {
        std::istringstream is{from};
        To to;
        is >> std::boolalpha >> to;
        ok = !is.fail();
        return to;

    };
    return rtti::MetaType::registerConverter<std::string, To>(lambda);
}

enum class operation { add, subtract, multiply, divide };

void register_global_ns()
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
        ._method("strToInt", &strToInt)
        ._enum<operation>("operation")
            ._element("add", operation::add)
            ._element("subtract", operation::subtract)
            ._element("multiply", operation::multiply)
            ._element("divide", operation::divide)
   ;

    // default convert
    rtti::MetaType::registerConverter<char*, std::string>();
    rtti::MetaType::registerConverter<char*, std::string_view>();
    // this is ERROR! c_str method is not a converting method!
    //rtti::MetaType::registerConverter<std::string, char const*>(
    //    [] (std::string const &value) { return value.c_str(); });

    // Integer promotion
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

    register_toString_converter<bool>();
    register_toString_converter<int>();
    register_toString_converter<unsigned>();
    register_toString_converter<long>();
    register_toString_converter<unsigned long>();
    register_toString_converter<long long>();
    register_toString_converter<unsigned long long>();
    register_toString_converter<float>();
    register_toString_converter<double>();
    register_toString_converter<long double>();

    register_fromString_converter<bool>();
    register_fromString_converter<int>();
    register_fromString_converter<unsigned>();
    register_fromString_converter<long>();
    register_fromString_converter<unsigned long>();
    register_fromString_converter<long long>();
    register_fromString_converter<unsigned long long>();
    register_fromString_converter<float>();
    register_fromString_converter<double>();
    register_fromString_converter<long double>();
}

