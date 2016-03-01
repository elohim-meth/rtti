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

        ._class<test::Test1>("Test1")
            ._attribute("Description", "Absolutly empty class")
        ._end()

        ._class<test::Test2>("Test2") // warning! no implicitly declared move and deprecated copy!
            ._attribute("Description", "Has only user delared virtual destructor defined as default")
        ._end()

        ._class<test::Test3>("Test3")
            ._attribute("Description", "Has user declared copy and move defined as default")
        ._end()

//This class is impossible to register cause variant requires CopyConstructible or MoveConstructible
//      ._class<test::Test4>("Test4")
//          ._attribute("Description", "Unmovable default constructible class")
//      ._end()

        ._class<test::Test5>("Test5") // warning! move participates in overload and not degenerate to copy!
            ._attribute("Description", "Has user declared copy defined as default and user declared move defined as delete")
        ._end()

        ._class<test::Test6>("Test6")
            ._attribute("Description", "Has user declared copy defined as default and not declared move")
        ._end()

        ._class<test::Test7>("Test7")
            ._attribute("Description", "Has user declared move defined as default and not declared copy")
        ._end()

//This class is impossible to register cause variant requires CopyConstructible or MoveConstructible
//      ._class<test::Test8>("Test8")
//          ._attribute("Description", "Has user declared move defined as delete and not declared copy")
//      ._end()
        ._class<test::CopyAndMove>("CopyAndMove")
        ._end()
        ._class<test::CopyOnly>("CopyOnly")
        ._end()
        ._class<test::MoveOnly>("MoveOnly")
        ._end()

    ;
}

struct define_std_namespace
{
    template<typename T>
    static void define_std_string(rtti::meta_define<T> define)
    {
        using size_type = typename T::size_type;
        using value_type = typename T::value_type;
        using reference = typename T::reference;
        using const_reference = typename T::const_reference;
        using iterator = typename T::iterator;
        using const_iterator = typename T::const_iterator;

        define
            . template _constructor<value_type const*>()
            . template _constructor<value_type const*, size_type>()

            . template _method("size", &T::size)
            . template _method<void (T::*)(size_type)>("resize", &T::resize)
            . template _method("capacity", &T::capacity)
            . template _method("empty", &T::empty)
            . template _method("clear", &T::clear)
            . template _method("reserve", &T::reserve)

            . template _method("c_str", &T::c_str)

            . template _method<iterator (T::*)()>("begin", &T::begin)
            . template _method<const_iterator (T::*)() const>("begin", &T::begin)
            . template _method("cbegin", &T::cbegin)

            . template _method<iterator (T::*)()>("end", &T::end)
            . template _method<const_iterator (T::*)() const>("end", &T::end)
            . template _method("cend", &T::cend)

            . template _method<reference (T::*)(size_type)>("[]", &T::operator[])
            . template _method<const_reference (T::*)(size_type) const>("[]", &T::operator[])
            . template _method<reference (T::*)(size_type)>("at", &T::at)
            . template _method<const_reference (T::*)(size_type) const>("at", &T::at)

            . template _method<T& (T::*)(T const&)>("+=", &T::operator+=)
            . template _method<T& (T::*)(value_type const*)>("+=", &T::operator+=)
            . template _method<T& (T::*)(value_type)>("+=", &T::operator+=)

            . template _method<size_type (T::*)(value_type const*, size_type) const>("find", &T::find)
            . template _method<size_type (T::*)(T const&, size_type) const>("find", &T::find)
            . template _method<size_type (T::*)(value_type, size_type) const>("find", &T::find)

            . template _method<size_type (T::*)(value_type const*, size_type) const>("rfind", &T::rfind)
            . template _method<size_type (T::*)(T const&, size_type) const>("rfind", &T::rfind)
            . template _method<size_type (T::*)(value_type, size_type) const>("rfind", &T::rfind)
        ;
    }

    void operator()(rtti::meta_define<void> define)
    {
        define
            ._attribute("Description", "Standard C++ library")
            ._class<std::string>("string")
                ._lazy(define_std_string<std::string>)
            ._end()
//            ._class<std::wstring>("wstring")
//                ._lazy(define_std_string<std::wstring>)
//            ._end()
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
                [] (const char *value)
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
