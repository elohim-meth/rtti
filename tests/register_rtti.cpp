#include <rtti/metadefine.h>

std::string g_string = "";
std::string const gro_string = "Hello, World";

std::string intToStr(int value, bool &ok)
{
    ok = true;
    return std::to_string(value);
}

void register_rtti()
{
    using namespace std::literals;

    rtti::global_define()
        ._attribute("Description", "Global namespace")
        ._attribute("Attribute 1", "standard string object"s)
        ._attribute("Attribute 2", true)
        ._attribute("Attribute 3", 3.14)
        ._property("g_string", &g_string)
        ._property("gro_string", &gro_string)
        ._method("intToStr", &intToStr)
   ;

    // default convert
    rtti::MetaType::registerConverter<char*, std::string>();

    rtti::MetaType::registerConverter<bool, char>();
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

