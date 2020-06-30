#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>
#include <rtti/metadefine.h>

namespace test {

struct DiamondTop
{
    DECLARE_CLASSINFO
public:
    int m_top = 256;
};

struct DiamondLeft : virtual DiamondTop
{
    DECLARE_CLASSINFO
public:
    double m_left = 3.14;
};

struct DiamondRight : virtual DiamondTop
{
    DECLARE_CLASSINFO
public:
    std::string m_right = "Hello, World!";
};

struct DiamondBottom : DiamondLeft, DiamondRight
{
    DECLARE_CLASSINFO
public:
    int m_bottom = 1024;
};

} // namespace test


RTTI_REGISTER
{
    rtti::global_define()
        ._namespace("test")
            ._class<test::DiamondTop>("DiamondTop")
            ._end()
            ._class<test::DiamondLeft>("DiamondLeft")
                ._base<test::DiamondTop>()
            ._end()
            ._class<test::DiamondRight>("DiamondRight")
                ._base<test::DiamondTop>()
            ._end()
            ._class<test::DiamondBottom>("DiamondBottom")
                ._base<test::DiamondLeft, test::DiamondRight>()
            ._end()
        ._end()
    ;
}

TEST_CASE("Virtual inheritance")
{
    test::DiamondBottom bottom;
    test::DiamondTop &ref_top = bottom;
    test::DiamondTop *ptr_top = &bottom;

    SUBCASE("Up-Casting")
    {
        REQUIRE(rtti::meta_cast<test::DiamondLeft>(&bottom));
        REQUIRE(rtti::meta_cast<test::DiamondRight>(&bottom));
        REQUIRE(rtti::meta_cast<test::DiamondTop>(&bottom));

        REQUIRE_NOTHROW(rtti::meta_cast<test::DiamondLeft>(bottom));
        REQUIRE_NOTHROW(rtti::meta_cast<test::DiamondRight>(bottom));
        REQUIRE_NOTHROW(rtti::meta_cast<test::DiamondTop>(bottom));
    }

    SUBCASE("Down-Casting")
    {
        REQUIRE(rtti::meta_cast<test::DiamondLeft>(ptr_top));
        REQUIRE(rtti::meta_cast<test::DiamondRight>(ptr_top));
        REQUIRE(rtti::meta_cast<test::DiamondBottom>(ptr_top));

        REQUIRE_NOTHROW(rtti::meta_cast<test::DiamondLeft>(ref_top));
        REQUIRE_NOTHROW(rtti::meta_cast<test::DiamondRight>(ref_top));
        REQUIRE_NOTHROW(rtti::meta_cast<test::DiamondBottom>(ref_top));
    }

    SUBCASE("Variant transformation")
    {
        auto *nsGlobal = rtti::MetaNamespace::global();
        REQUIRE(nsGlobal);
        auto *nsTest = nsGlobal->getNamespace("test");
        REQUIRE(nsTest);
        auto *mcBottom = nsTest->getClass("DiamondBottom");
        REQUIRE(mcBottom);
        auto *constructor = mcBottom->defaultConstructor();
        REQUIRE(constructor);
        auto instance = constructor->invoke();

        REQUIRE(instance.is<test::DiamondBottom>());
        REQUIRE(instance.is<test::DiamondLeft>());
        REQUIRE(instance.is<test::DiamondRight>());
        REQUIRE(instance.is<test::DiamondTop>());
    }

}
