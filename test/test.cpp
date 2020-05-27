#define CATCH_CONFIG_RUNNER

#include <catch2/catch.hpp>
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

TEST_CASE("Test global namespace", "[global]")
{
    auto nsGlobal = rtti::MetaNamespace::global();
    REQUIRE(nsGlobal);

    SECTION("Test global string property")
    {
        auto property = nsGlobal->getProperty("g_string");
        REQUIRE(property);
        REQUIRE_THROWS(property->get().rref<std::string>().empty());
        REQUIRE(property->get().ref<std::string>().empty());
        REQUIRE(property->get().ref<std::string const>().empty());
        REQUIRE(property->get().cref<std::string>().empty());
        property->set("Asdfg");
        REQUIRE(g_string == "Asdfg");
        property->set(std::string{"Qwerty"});
        REQUIRE(g_string == "Qwerty");
    }
}

int main(int argc, char* argv[])
{
    // global setup...
    register_rtti();

    auto result = Catch::Session().run(argc, argv);
    return result;
}
