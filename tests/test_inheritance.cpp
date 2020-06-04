#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>
#include <rtti/metadefine.h>

namespace test {

struct BaseA
{
    DECLARE_CLASSINFO
public:
    int m_basea = 256;
};

struct SingleA1: BaseA
{
    DECLARE_CLASSINFO
public:
    bool m_singlea1 = true;
};

struct SingleA2: SingleA1
{
    DECLARE_CLASSINFO
public:
    double m_singlea2 = 3.14;
};

struct SingleA22: SingleA1
{
    DECLARE_CLASSINFO
public:
    double m_singlea2 = 2.17;
};

struct SingleA3: SingleA2
{
    DECLARE_CLASSINFO
public:
    std::string m_singlea3 = "Hello, World!";
};

struct SingleA33: SingleA2
{
    DECLARE_CLASSINFO
public:
    std::string m_singlea3 = "Good bye!";
};

struct SingleA4: SingleA3
{
    DECLARE_CLASSINFO
public:
    int64_t m_singlea4 = -1;
};

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

bool test_class_param(SingleA2 const *param)
{
    return
        (param->m_basea == 256) &&
        (param->m_singlea1 == true) &&
        (param->m_singlea2 == 3.14);
}

bool test_class_param(SingleA2 const &param)
{
    return
        (param.m_basea == 256) &&
        (param.m_singlea1 == true) &&
        (param.m_singlea2 == 3.14);
}

bool test_class_param(SingleA2 *param)
{
    param->m_singlea1 = false;
    return
        (param->m_basea == 256) &&
        (param->m_singlea1 == false) &&
        (param->m_singlea2 == 3.14);
}

bool test_class_param(SingleA2 &param)
{
    param.m_singlea1 = false;
    return
        (param.m_basea == 256) &&
        (param.m_singlea1 == false) &&
        (param.m_singlea2 == 3.14);
}


} // namespace test


RTTI_REGISTER
{
    rtti::global_define()
        ._namespace("test")
            ._class<test::BaseA>("BaseA")
            ._end()
            ._class<test::SingleA1>("SingleA1")
                ._base<test::BaseA>()
            ._end()
            ._class<test::SingleA2>("SingleA2")
                ._base<test::SingleA1>()
            ._end()
            ._class<test::SingleA22>("SingleA22")
                ._base<test::SingleA1>()
            ._end()
            ._class<test::SingleA3>("SingleA3")
                ._base<test::SingleA2>()
            ._end()
            ._class<test::SingleA33>("SingleA33")
                ._base<test::SingleA2>()
            ._end()
            ._class<test::SingleA4>("SingleA4")
                ._base<test::SingleA3>()
            ._end()

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

            ._method<bool(*)(test::SingleA2 const *)>("test_class_param [const ptr]", &test::test_class_param)
            ._method<bool(*)(test::SingleA2 *)>("test_class_param [ptr]", &test::test_class_param)
            ._method<bool(*)(test::SingleA2 const &)>("test_class_param [const ref]", &test::test_class_param)
            ._method<bool(*)(test::SingleA2 &)>("test_class_param [ref]", &test::test_class_param)

        ._end()
    ;
}

TEST_CASE("Single inheritance")
{
    test::SingleA3 derived;
    test::SingleA3 const &const_derived = derived;
    test::BaseA *ptr2base = &derived;
    test::BaseA const *const_ptr2base = &derived;
    test::BaseA &ref2base = derived;
    test::BaseA const &const_ref2base = derived;

    SUBCASE("Up-Casting")
    {
        REQUIRE(rtti::meta_cast<test::BaseA>(&derived));
        REQUIRE(rtti::meta_cast<test::SingleA1>(&derived));
        REQUIRE(rtti::meta_cast<test::SingleA2>(&derived));
        REQUIRE(rtti::meta_cast<test::SingleA3>(&derived));
        REQUIRE_FALSE(rtti::meta_cast<test::SingleA22>(&derived));
        REQUIRE_FALSE(rtti::meta_cast<test::SingleA33>(&derived));

        REQUIRE_NOTHROW(rtti::meta_cast<test::BaseA>(derived));
        REQUIRE_NOTHROW(rtti::meta_cast<test::SingleA1>(derived));
        REQUIRE_NOTHROW(rtti::meta_cast<test::SingleA2>(derived));
        REQUIRE_NOTHROW(rtti::meta_cast<test::SingleA3>(derived));
        REQUIRE_THROWS(rtti::meta_cast<test::SingleA22>(derived));
        REQUIRE_THROWS(rtti::meta_cast<test::SingleA33>(derived));
    }


    SUBCASE("Down-Casting")
    {
        REQUIRE(rtti::meta_cast<test::BaseA>(ptr2base));
        REQUIRE(rtti::meta_cast<test::SingleA1>(ptr2base));
        REQUIRE(rtti::meta_cast<test::SingleA2>(ptr2base));
        REQUIRE(rtti::meta_cast<test::SingleA3>(ptr2base));
        REQUIRE_FALSE(rtti::meta_cast<test::SingleA22>(ptr2base));
        REQUIRE_FALSE(rtti::meta_cast<test::SingleA33>(ptr2base));
        REQUIRE_FALSE(rtti::meta_cast<test::SingleA4>(ptr2base));

        REQUIRE_NOTHROW(rtti::meta_cast<test::BaseA>(ref2base));
        REQUIRE_NOTHROW(rtti::meta_cast<test::SingleA1>(ref2base));
        REQUIRE_NOTHROW(rtti::meta_cast<test::SingleA2>(ref2base));
        REQUIRE_NOTHROW(rtti::meta_cast<test::SingleA3>(ref2base));
        REQUIRE_THROWS(rtti::meta_cast<test::SingleA22>(ref2base));
        REQUIRE_THROWS(rtti::meta_cast<test::SingleA33>(ref2base));
        REQUIRE_THROWS(rtti::meta_cast<test::SingleA4>(ref2base));
    }

    SUBCASE("Param transformation")
    {
        auto *nsGlobal = rtti::MetaNamespace::global();
        REQUIRE(nsGlobal);
        auto *nsTest = nsGlobal->getNamespace("test");
        REQUIRE(nsTest);

        SUBCASE("Pass by pointer to const base")
        {
            auto *method = nsTest->getMethod("test_class_param [const ptr]");
            REQUIRE(method);
            auto v = method->invoke(rtti::variant(&derived));
            REQUIRE(v == true);

        }
    }
}
