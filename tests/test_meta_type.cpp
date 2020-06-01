#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>

#include <rtti/metatype.h>

TEST_CASE("Find metatype by type name")
{
    auto typeId = rtti::metaTypeId<long double>();
    REQUIRE(rtti::MetaType{rtti::type_name<long double>()}.typeId() == typeId);

    typeId = rtti::metaTypeId<char const[256]>();
    REQUIRE(rtti::MetaType{rtti::type_name<char const[256]>()}.typeId() == typeId);

    typeId = rtti::metaTypeId<std::string>();
    REQUIRE(rtti::MetaType{rtti::type_name<std::string>()}.typeId() == typeId);

    // Type not found until rtti::metaTypeId<>() is called
    REQUIRE_FALSE(rtti::MetaType{rtti::type_name<void *****&>()}.valid());

    // Now type is registered
    typeId = rtti::metaTypeId<void *****&>();
    REQUIRE(rtti::MetaType{rtti::type_name<void *****&>()}.valid());
    REQUIRE(rtti::MetaType{rtti::type_name<void *****&>()}.typeId() == typeId);
}

TEST_CASE("Test type size")
{
    REQUIRE(rtti::metaType<void>().typeSize() == 0);
    REQUIRE(rtti::metaType<bool>().typeSize() == 1);
    REQUIRE(rtti::metaType<char>().typeSize() == 1);
    REQUIRE(rtti::metaType<int16_t>().typeSize() == 2);
    REQUIRE(rtti::metaType<int32_t>().typeSize() == 4);
    REQUIRE(rtti::metaType<int64_t>().typeSize() == 8);
    REQUIRE(rtti::metaType<void(*)(int)>().typeSize() == sizeof(size_t));
    REQUIRE(rtti::metaType<void*>().typeSize() == sizeof(size_t));
}

