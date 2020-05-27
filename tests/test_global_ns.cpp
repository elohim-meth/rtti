#include <catch2/catch.hpp>
#include <rtti/metadefine.h>

extern std::string g_string;
extern std::string gro_string;

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

    SECTION("Test global readonly string property")
    {
        auto property = nsGlobal->getProperty("g_string");
        REQUIRE(property);

    }
}
