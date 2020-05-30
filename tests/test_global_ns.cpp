﻿#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
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
            REQUIRE_THROWS(v.ref<std::string>());
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
            REQUIRE_THROWS(v.ref<int64_t>());
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
        REQUIRE_THROWS(property->get().rref<std::string>());
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
        REQUIRE_THROWS(v.ref<std::string>());
        REQUIRE(v.cref<std::string>() == "Hello, World!");
        REQUIRE_THROWS(v.ref<std::string>() = "Test");
        REQUIRE_THROWS(property->set("Test"s));
    }

    SUBCASE("Test global function")
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
            REQUIRE(((v == "123"s) && ok.ref<bool>()));
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
            REQUIRE_THROWS(method->invoke(123, false));
            REQUIRE_THROWS(method->invoke(123, ok));
            REQUIRE_THROWS(method->invoke(123, v_ok));
            REQUIRE_THROWS(method->invoke(123, v_ok1));
            REQUIRE_THROWS(method->invoke(123, v_ok2));
            rtti::variant number = 123.12;
            REQUIRE_THROWS(method->invoke("ABC", ok1));
            REQUIRE_THROWS(method->invoke(3.14, ok1));
            REQUIRE_THROWS(method->invoke(number, ok1));
        }

    }


}