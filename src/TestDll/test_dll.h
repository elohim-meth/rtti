#include "test_register.h"
#include "test_variant.h"

#include <debug.h>

namespace test {

enum class Color {
    Red,
    Green,
    Blue
};

class Absolute {};

class Empty
{
public:
    Empty() noexcept
    { PRINT_PRETTY_FUNC }
    Empty(const Empty&)  noexcept
    { PRINT_PRETTY_FUNC }
    Empty& operator=(const Empty&) noexcept
    {
        PRINT_PRETTY_FUNC
        return *this;
    }
    Empty(Empty&&) noexcept
    { PRINT_PRETTY_FUNC }
    Empty& operator=(Empty&&) noexcept
    {
        PRINT_PRETTY_FUNC
        return *this;
    }
    ~Empty() noexcept
    { PRINT_PRETTY_FUNC }
};

class Small{
public:
    explicit Small() noexcept : i{-1}
    { PRINT_PRETTY_FUNC }
    explicit Small(std::int8_t v) noexcept : i{v}
    { PRINT_PRETTY_FUNC }
    Small(const Small &v) noexcept : i{v.i}
    { PRINT_PRETTY_FUNC }
    Small& operator=(const Small&v) noexcept
    {
        PRINT_PRETTY_FUNC
        i = v.i;
        return *this;
    }
    Small(Small&&v) noexcept : i{v.i}
    {
        PRINT_PRETTY_FUNC
        v.i = 0;
    }
    Small& operator=(Small &&v) noexcept
    {
        PRINT_PRETTY_FUNC
        i = v.i;
        v.i = 0;
        return *this;
    }
    virtual ~Small() noexcept
    {
        PRINT_PRETTY_FUNC
    }

    std::int8_t i;
};

//struct Big
//{
//    Big(std::size_t v1 = 0, std::size_t v2 = 0)
//        : i1{v1}, i2{v2}
//    {
//        PRINT_PRETTY_FUNC
//    }
//    Big(const Big &v): i1{v.i1}, i2{v.i2}
//    {
//        PRINT_PRETTY_FUNC
//    }
//    Big& operator=(const Big &v)
//    {
//        PRINT_PRETTY_FUNC
//        i1 = v.i1;
//        i2 = v.i2;
//        return *this;
//    }
//    Big(Big &&v) noexcept : i1{v.i1}, i2{v.i2}
//    {
//        std::cout << __PRETTY_FUNCTION__ << std::endl;
//        v.i1 = v.i2 = 0;
//    }
//    Big& operator=(Big &&v) noexcept
//    {
//        PRINT_PRETTY_FUNC
//        i1 = v.i1;
//        i2 = v.i2;
//        v.i1 = v.i2 = 0;
//        return *this;
//    }
//    ~Big()
//    {
//        PRINT_PRETTY_FUNC
//    }

//    std::size_t i1;
//    std::size_t i2;
//};

class TestBase1 {
public:
    TestBase1() { PRINT_PRETTY_FUNC }
    TestBase1(const TestBase1&) { PRINT_PRETTY_FUNC }
    TestBase1(TestBase1&&) { PRINT_PRETTY_FUNC }
    TestBase1(int, const std::string&){ PRINT_PRETTY_FUNC }
    virtual ~TestBase1(){ PRINT_PRETTY_FUNC }
    enum TestEnum {
        te1 = 10,
        te2 = 20
    };

};
class TestBase2 {};
class TestDerived: public TestBase1, public TestBase2 {};

} // namespace test
