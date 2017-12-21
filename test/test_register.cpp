#include "test_register.h"

namespace {

struct define_std_namespace
{
    void operator()(rtti::meta_define<void> define)
    {
        define
            ._attribute("Description", "Standard C++ library")
            ._class<std::string>("string")
                ._lazy(define_std_string<std::string>)
            ._end()
            ._class<std::wstring>("wstring")
                ._lazy(define_std_string<std::wstring>)
            ._end()
//            ._class<std::u16string>("u16string")
//                ._lazy(define_std_string<std::u16string>)
//            ._end()
//            ._class<std::u32string>("u32string")
//                ._lazy(define_std_string<std::u32string>)
//            ._end()
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

std::string global_string = "";
std::string const global_readonly_string = "Hello, World";

std::string intToStr(int value, bool &ok)
{
    ok = true;
    return std::to_string(value);
}

} // namespace

void register_rtti()
{
    rtti::global_define()
        ._attribute("Description", "Global namespace")
        ._attribute("Attribute 1", std::string{"standard string object"})
        ._attribute("Attribute 2", true)
        ._attribute("Attribute 3", 3.14)
        ._property("global_string", &global_string)
        ._property("global_readonly_string", &global_readonly_string)
        ._method("intToStr", &intToStr)

        ._namespace("test")
            ._lazy(define_test_namespace)
        ._end()

        ._namespace("std")
            ._lazy(define_std_namespace{})
        ._end();
    ;

    register_std_vector<int>();
    register_std_vector<std::string>();
    register_std_vector<char const*>();
    register_std_unique_ptr<int>();
    register_std_unique_ptr<int const>();

    // default
    rtti::MetaType::registerConverter<char*, std::string>();
    rtti::MetaType::registerConverter<bool, char>();
    rtti::MetaType::registerConverter<int, unsigned int>();
    rtti::MetaType::registerConverter<unsigned int, int>();
    rtti::MetaType::registerConverter<int, long int>();
    rtti::MetaType::registerConverter<long int, int>();
    rtti::MetaType::registerConverter<int, long long int>();
    rtti::MetaType::registerConverter<float, double>();
    rtti::MetaType::registerConverter<float, long double>();
    rtti::MetaType::registerConverter<double, long double>();

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
    rtti::MetaType::registerConverter<std::string, long>(
                [] (const std::string &value)
                {
                    return std::stol(value);
                });
    rtti::MetaType::registerConverter<std::string, unsigned long>(
                [] (const std::string &value)
                {
                    return std::stoul(value);
                });
    rtti::MetaType::registerConverter<std::string, long long>(
                [] (const std::string &value)
                {
                    return std::stoll(value);
                });
    rtti::MetaType::registerConverter<std::string, unsigned long long>(
                [] (const std::string &value)
                {
                    return std::stoull(value);
                });
    rtti::MetaType::registerConverter<std::string, float>(
                [] (const std::string &value)
                {
                    return std::stof(value);
                });
    rtti::MetaType::registerConverter<std::string, double>(
                [] (const std::string &value)
                {
                    return std::stod(value);
                });
    rtti::MetaType::registerConverter<char*, double>(
                [] (char const *value)
                {
                    char *end;
                    return std::strtod(value, &end);
                });

    rtti::MetaType::registerConverter<std::string, long double>(
                [] (const std::string &value)
                {
                    return std::stold(value);
                });

    // std::ostringstream
    register_fromString_converter<bool>();

}
