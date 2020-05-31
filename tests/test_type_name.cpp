#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>

#include <rtti/typename.h>
#include <rtti/metatype.h>

TEST_CASE("Test type name generation")
{
   REQUIRE(rtti::type_name<int*>() == "int*");
   REQUIRE(rtti::type_name<int const*>() == "const int*");
   REQUIRE(rtti::type_name<int&>() == "int&");
   REQUIRE(rtti::type_name<int const**&>() == "const int**&");
}


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
