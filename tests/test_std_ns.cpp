#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>
#include <rtti/metadefine.h>

TEST_CASE("vector of int")
{
    auto il = {1, 2, 3, 4, 5, 6, 7};
    rtti::variant tmp = il;

    auto *meta_class = rtti::MetaNamespace::global()->getNamespace("std")->getClass("vector<int>");
    REQUIRE(meta_class);
}
