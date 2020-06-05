#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>
#include <rtti/metadefine.h>

extern std::string g_string;
extern std::string gro_string;

TEST_CASE("Test global namespace")
{
    using namespace std::literals;
    auto ns_Global = rtti::MetaNamespace::global();
    REQUIRE(ns_Global);

    SUBCASE("Test attributes")
    {
        REQUIRE(ns_Global->attributeCount() == 6);

        SUBCASE("C-string attribute")
        {
            REQUIRE(ns_Global->attributeName(0) == "Description");
            auto &v = ns_Global->attribute("Description");
            REQUIRE(v.to<std::string>() == "Global namespace");
            REQUIRE(v.to<std::string_view>() == "Global namespace");
            REQUIRE(v == rtti::variant{"Global namespace"});
            REQUIRE(v == "Global namespace");
            REQUIRE_THROWS_AS(v.ref<std::string>(), rtti::bad_variant_cast);
            REQUIRE(std::strcmp(v.ref<char const*>(), "Global namespace") == 0);
        }

        SUBCASE("std::string attribute")
        {
            REQUIRE(ns_Global->attributeName(1) == "string_attr");
            auto &v = ns_Global->attribute("string_attr");
            REQUIRE(v.ref<std::string>() == "Hello, World!");
            REQUIRE(v == "Hello, World!"s);
        }

        SUBCASE("Boolean attribute")
        {
            REQUIRE(ns_Global->attributeName(2) == "bool_attr");
            auto &v = ns_Global->attribute("bool_attr");
            REQUIRE(v.ref<bool>() == true);
            REQUIRE(v == true);
        }

        SUBCASE("Double attribute")
        {
            REQUIRE(ns_Global->attributeName(3) == "double_attr");
            auto &v = ns_Global->attribute("double_attr");
            REQUIRE(v.ref<double>() == 3.14);
            REQUIRE(v == 3.14);
        }

        SUBCASE("Integer attribute")
        {
            REQUIRE(ns_Global->attributeName(4) == "int_attr");
            auto &v = ns_Global->attribute("int_attr");
            REQUIRE(v.ref<int>() == 256);
            REQUIRE_THROWS_AS(v.ref<int64_t>(), rtti::bad_variant_cast);
            REQUIRE(v.to<int64_t>() == 256);
            REQUIRE(v == 256);
            REQUIRE_FALSE(v == "256"s);
            REQUIRE(v.eq("256"s));
            REQUIRE_FALSE(v.eq("ABC"s));
        }

        SUBCASE("Enum attribute")
        {
            auto m_enum = ns_Global->getEnum("operation");
            REQUIRE(m_enum);

            REQUIRE(ns_Global->attributeName(5) == "enum_attr");
            auto &v = ns_Global->attribute("enum_attr");
            REQUIRE(v == m_enum->element("multiply"));
        }

    }

    SUBCASE("Test global string property")
    {
        auto property = ns_Global->getProperty("g_string");
        REQUIRE(property);
        REQUIRE_THROWS_AS(property->get().rref<std::string>(), rtti::bad_variant_cast);
        REQUIRE(property->get().ref<std::string>().empty());
        REQUIRE(property->get().ref<std::string const>().empty());
        REQUIRE(property->get().cref<std::string>().empty());
        property->set("Asdfg");
        REQUIRE(g_string == "Asdfg");
        property->set("Qwerty"s);
        REQUIRE(g_string == "Qwerty");
    }

    SUBCASE("Test global readonly string property")
    {
        auto property = ns_Global->getProperty("gro_string");
        REQUIRE(property);
        REQUIRE(property->get().ref<std::string>() == "Hello, World!");
        REQUIRE(property->get() == "Hello, World!"s);

        auto v = property->get();
        REQUIRE_THROWS_AS(v.ref<std::string>(), rtti::bad_variant_cast);
        REQUIRE(v.cref<std::string>() == "Hello, World!");
        REQUIRE_THROWS_AS(v.ref<std::string>() = "Test", rtti::bad_variant_cast);
        REQUIRE_THROWS_AS(property->set("Test"s), rtti::invoke_error);
    }

    SUBCASE("Test global function [intToStr]")
    {
        auto method = ns_Global->getMethod("intToStr");
        REQUIRE(method);
        SUBCASE("Test invoke 1")
        {
            bool ok = false;
            auto v = method->invoke(123, ok);
            REQUIRE(((v.ref<std::string>() == "123") && ok));
            REQUIRE(((v == "123"s) && ok));
        }
        SUBCASE("Test invoke 2")
        {
            rtti::variant ok = false;
            rtti::variant number = 123;
            auto v = method->invoke(number, ok);
            REQUIRE(((v == "123"s) && ok.ref<bool>() && (ok == true)));

            int i = 256;
            number = std::ref(i);
            v = method->invoke(number, ok);
            REQUIRE(((v == "256"s) && ok.ref<bool>() && (ok == true)));

            number = std::cref(i);
            v = method->invoke(number, ok);
            REQUIRE(((v == "256"s) && ok.ref<bool>() && (ok == true)));
        }
        SUBCASE("Test invoke 3")
        {
            bool ok = false;
            rtti::variant v_ok = std::ref(ok);
            auto v = method->invoke(123, ok);
            REQUIRE(((v.ref<std::string>() == "123") && ok));
            REQUIRE(((v == "123"s) && ok));
        }
        SUBCASE("Test invoke 4")
        {
            bool const ok = false;
            rtti::variant v_ok = std::ref(ok);
            bool ok1 = false;
            rtti::variant const v_ok1 = false;
            rtti::variant v_ok2 = std::cref(ok1);;
            REQUIRE_THROWS_AS(method->invoke(123, false), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(123, ok), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(123, v_ok), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(123, v_ok1), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(123, v_ok2), rtti::bad_variant_cast);
            rtti::variant number = 123.12;
            REQUIRE_THROWS_AS(method->invoke("ABC", ok1), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(3.14, ok1), rtti::bad_variant_cast);
            REQUIRE_THROWS_AS(method->invoke(number, ok1), rtti::bad_variant_cast);
        }

    }

    SUBCASE("Test global function [strToInt]")
    {
        auto method = ns_Global->getMethod("strToInt");
        REQUIRE(method);
        SUBCASE("Test invoke 1")
        {
            bool ok = false;
            auto v = method->invoke("256", ok);
            REQUIRE((ok && (v == 256)));
            REQUIRE(v.ref<int>() == 256);
            REQUIRE(v.to<int>() == 256);
        }

        SUBCASE("Test invoke 2")
        {
            bool ok = false;
            auto v = method->invoke("256"s, ok);
            REQUIRE((ok && (v == 256)));
        }

        SUBCASE("Test invoke 3")
        {
            bool ok = false;
            rtti::variant str = "1024";
            auto v = method->invoke(str, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 4")
        {
            bool ok = false;
            rtti::variant str = "1024"s;
            auto v = method->invoke(str, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 5")
        {
            bool ok = false;
            rtti::variant const str = "1024";
            auto v = method->invoke(str, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 6")
        {
            bool ok = false;
            char const* str = "1024";
            rtti::variant v_str = str;
            auto v = method->invoke(str, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 7")
        {
            bool ok = false;
            char str[] = "1024";
            rtti::variant v_str = str;
            auto v = method->invoke(str, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 8")
        {
            bool ok = false;
            char str[] = "1024";
            rtti::variant v_str = std::ref(str);
            auto v = method->invoke(v_str, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 9")
        {
            bool ok = false;
            char str[] = "1024";
            rtti::variant v_str = std::cref(str);
            auto v = method->invoke(v_str, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 10")
        {
            bool ok = false;
            auto v = method->invoke(rtti::variant{"1024"}, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 11")
        {
            bool ok = false;
            auto v = method->invoke(rtti::variant{"1024"s}, ok);
            REQUIRE((ok && (v == 1024)));
        }

        SUBCASE("Test invoke 12")
        {
            bool ok = false;
            auto v = method->invoke("Q123A", ok);
            REQUIRE_FALSE(ok);
        }

        // This will work cause of registered converter from int to string
        SUBCASE("Test invoke 13")
        {
            bool ok = false;
            auto v = method->invoke(2048, ok);
            REQUIRE((ok && (v == 2048)));
        }
    }
}
