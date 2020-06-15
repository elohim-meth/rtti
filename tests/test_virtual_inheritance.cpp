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
}
