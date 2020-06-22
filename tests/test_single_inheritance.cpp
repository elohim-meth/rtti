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

int test_class_param(SingleA2 const *param)
{
    if ((param->m_basea == 256) && (param->m_singlea1 == true) && (param->m_singlea2 == 3.14))
        return 1;
    return 0;
}

int test_class_param(SingleA2 *param)
{
    param->m_singlea1 = false;
    if ((param->m_basea == 256) && (param->m_singlea1 == false) && (param->m_singlea2 == 3.14))
        return 1;
    return 0;
}

int test_class_param(SingleA2 const &param)
{
    if ((param.m_basea == 256) && (param.m_singlea1 == true) && (param.m_singlea2 == 3.14))
        return 1;
    return 0;
}

int test_class_param(SingleA2 &param)
{
    param.m_singlea1 = false;
    if ((param.m_basea == 256) && (param.m_singlea1 == false) && (param.m_singlea2 == 3.14))
        return 1;
    return 0;
}

bool test_class_param(SingleA2 &&param)
{
    return true;
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

            ._method<int(*)(test::SingleA2 const *)>("test_class_param [const ptr]", &test::test_class_param)
            ._method<int(*)(test::SingleA2 *)>("test_class_param [ptr]", &test::test_class_param)
            ._method<int(*)(test::SingleA2 const &)>("test_class_param [const ref]", &test::test_class_param)
            ._method<int(*)(test::SingleA2 &)>("test_class_param [ref]", &test::test_class_param)
            ._method<bool(*)(test::SingleA2 &&)>("test_class_param [rref]", &test::test_class_param)

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

    rtti::variant v1  = const_ptr2base;
    rtti::variant v11 = std::ref(const_ptr2base);
    rtti::variant v2  = ptr2base;
    rtti::variant v22 = std::ref(ptr2base);

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
        REQUIRE_THROWS_AS(rtti::meta_cast<test::SingleA22>(derived), rtti::bad_meta_cast);
        REQUIRE_THROWS_AS(rtti::meta_cast<test::SingleA33>(derived), rtti::bad_meta_cast);
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
        REQUIRE_THROWS_AS(rtti::meta_cast<test::SingleA22>(ref2base), rtti::bad_meta_cast);
        REQUIRE_THROWS_AS(rtti::meta_cast<test::SingleA33>(ref2base), rtti::bad_meta_cast);
        REQUIRE_THROWS_AS(rtti::meta_cast<test::SingleA4>(ref2base), rtti::bad_meta_cast);
    }

    SUBCASE("Variant transformation")
    {
        auto *nsGlobal = rtti::MetaNamespace::global();
        REQUIRE(nsGlobal);
        auto *nsTest = nsGlobal->getNamespace("test");
        REQUIRE(nsTest);
        auto *mcSingleA3 = nsTest->getClass("SingleA3");
        REQUIRE(mcSingleA3);
        auto *constructor = mcSingleA3->defaultConstructor();
        REQUIRE(constructor);
        auto instance = constructor->invoke();
        REQUIRE(instance.is<test::BaseA>());
        REQUIRE(instance.is<test::SingleA1>());
        REQUIRE(instance.is<test::SingleA2>());
        REQUIRE(instance.is<test::SingleA3>());
        REQUIRE_FALSE(instance.is<test::SingleA22>());
        REQUIRE_FALSE(instance.is<test::SingleA33>());
        REQUIRE_FALSE(instance.is<test::SingleA4>());
    }

    SUBCASE("Param transformation")
    {
        auto *nsGlobal = rtti::MetaNamespace::global();
        REQUIRE(nsGlobal);
        auto *nsTest = nsGlobal->getNamespace("test");
        REQUIRE(nsTest);
        derived.m_singlea1 = true;

        SUBCASE("Pass as pointer to const base")
        {
            rtti::variant v1  = const_ptr2base;
            rtti::variant v11 = std::ref(const_ptr2base);
            rtti::variant v2  = ptr2base;
            rtti::variant v22 = std::ref(ptr2base);

            auto *method = nsTest->getMethod("test_class_param [const ptr]");
            REQUIRE(method);

            auto v = method->invoke(&derived);
            REQUIRE(v == 1);
            v = method->invoke(rtti::variant(&derived));
            REQUIRE(v == 1);

            v = method->invoke(&const_derived);
            REQUIRE(v == 1);
            v = method->invoke(ptr2base);
            REQUIRE(v == 1);
            v = method->invoke(const_ptr2base);
            REQUIRE(v == 1);
            v = method->invoke(&ref2base);
            REQUIRE(v == 1);
            v = method->invoke(&const_ref2base);
            REQUIRE(v == 1);

            REQUIRE_THROWS_AS(method->invoke(derived), rtti::bad_variant_cast);

            v = method->invoke(v1);
            REQUIRE(v == 1);
            v = method->invoke(v11);
            REQUIRE(v == 1);
            v = method->invoke(v2);
            REQUIRE(v == 1);
            v = method->invoke(v22);
            REQUIRE(v == 1);

            test::SingleA22 temp;
            REQUIRE_THROWS_AS(method->invoke(&temp), rtti::bad_variant_cast);
        }

        SUBCASE("Pass as pointer to base")
        {
            rtti::variant v1  = const_ptr2base;
            rtti::variant v11 = std::ref(const_ptr2base);
            rtti::variant v2  = ptr2base;
            rtti::variant v22 = std::ref(ptr2base);

            auto *method = nsTest->getMethod("test_class_param [ptr]");
            REQUIRE(method);

            derived.m_singlea1 = true;
            auto v = method->invoke(&derived);
            REQUIRE(((v == 1) && !derived.m_singlea1));

            derived.m_singlea1 = true;
            v = method->invoke(rtti::variant(&derived));
            REQUIRE(((v == 1) && !derived.m_singlea1));

            REQUIRE_THROWS_AS(method->invoke(&const_derived), rtti::bad_variant_cast);

            derived.m_singlea1 = true;
            v = method->invoke(ptr2base);
            REQUIRE(((v == 1) && !derived.m_singlea1));

            REQUIRE_THROWS_AS(method->invoke(const_ptr2base), rtti::bad_variant_cast);

            derived.m_singlea1 = true;
            v = method->invoke(&ref2base);
            REQUIRE(((v == 1) && !derived.m_singlea1));

            REQUIRE_THROWS_AS(method->invoke(&const_ref2base), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(v1), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(v11), rtti::bad_variant_cast);

            derived.m_singlea1 = true;
            v = method->invoke(v2);
            REQUIRE(((v == 1) && !derived.m_singlea1));

            derived.m_singlea1 = true;
            v = method->invoke(v22);
            REQUIRE(((v == 1) && !derived.m_singlea1));
        }

        SUBCASE("Pass as reference to const base")
        {
            rtti::variant v1  = std::ref(derived);
            rtti::variant v11 = std::ref(const_derived);
            rtti::variant v2  = std::ref(ref2base);
            rtti::variant v22 = std::ref(const_ref2base);

            auto *method = nsTest->getMethod("test_class_param [const ref]");
            REQUIRE(method);
            auto v = method->invoke(derived);
            REQUIRE(v == 1);
            v = method->invoke(rtti::variant(derived));
            REQUIRE(v == 1);

            v = method->invoke(const_derived);
            REQUIRE(v == 1);
            v = method->invoke(*ptr2base);
            REQUIRE(v == 1);
            v = method->invoke(*const_ptr2base);
            REQUIRE(v == 1);
            v = method->invoke(ref2base);
            REQUIRE(v == 1);
            v = method->invoke(const_ref2base);
            REQUIRE(v == 1);

            REQUIRE_THROWS_AS(method->invoke(ptr2base), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(const_ptr2base), rtti::bad_variant_cast);

            v = method->invoke(v1);
            REQUIRE(v == 1);
            v = method->invoke(v11);
            REQUIRE(v == 1);
            v = method->invoke(v2);
            REQUIRE(v == 1);
            v = method->invoke(v22);
            REQUIRE(v == 1);
        }

        SUBCASE("Pass as reference to base")
        {
            rtti::variant v1  = std::ref(derived);
            rtti::variant v11 = std::ref(const_derived);
            rtti::variant v2  = std::ref(ref2base);
            rtti::variant v22 = std::ref(const_ref2base);

            auto *method = nsTest->getMethod("test_class_param [ref]");
            REQUIRE(method);

            derived.m_singlea1 = true;
            auto v = method->invoke(derived);
            REQUIRE(((v == 1) && !derived.m_singlea1));

            REQUIRE_THROWS_AS(method->invoke(rtti::variant(derived)), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(const_derived), rtti::bad_variant_cast);

            derived.m_singlea1 = true;
            v = method->invoke(*ptr2base);
            REQUIRE(((v == 1) && !derived.m_singlea1));
            REQUIRE_THROWS_AS(method->invoke(*const_ptr2base), rtti::bad_variant_cast);

            derived.m_singlea1 = true;
            v = method->invoke(ref2base);
            REQUIRE(((v == 1) && !derived.m_singlea1));
            REQUIRE_THROWS_AS(method->invoke(const_ref2base), rtti::bad_variant_cast);

            derived.m_singlea1 = true;
            v = method->invoke(ref2base);
            REQUIRE(((v == 1) && !derived.m_singlea1));
            REQUIRE_THROWS_AS(method->invoke(v11), rtti::bad_variant_cast);

            derived.m_singlea1 = true;
            v = method->invoke(v2);
            REQUIRE(((v == 1) && !derived.m_singlea1));
            REQUIRE_THROWS_AS(method->invoke(v22), rtti::bad_variant_cast);
        }
    }
}
