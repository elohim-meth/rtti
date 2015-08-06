#include "test_register.h"
#include "test_variant.h"
#include "test_cast.h"

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
    Empty()
    { PRINT_PRETTY_FUNC }
    Empty(const Empty&)
    { PRINT_PRETTY_FUNC }
    Empty& operator=(const Empty&)
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
    ~Empty()
    { PRINT_PRETTY_FUNC }
};

class Small{
public:
    explicit Small() : i{-1}
    { PRINT_PRETTY_FUNC }
    explicit Small(std::int8_t v) : i{v}
    { PRINT_PRETTY_FUNC }
    Small(const Small &v) : i{v.i}
    { PRINT_PRETTY_FUNC }
    Small& operator=(const Small&v)
    {
        PRINT_PRETTY_FUNC
        i = v.i;
        return *this;
    }
    Small(Small&&v) noexcept : i{v.i}
    {
        PRINT_PRETTY_FUNC
        v.i = -1;
    }
    Small& operator=(Small &&v) noexcept
    {
        PRINT_PRETTY_FUNC
        i = v.i;
        v.i = -1;
        return *this;
    }
    virtual ~Small()
    {
        PRINT_PRETTY_FUNC
    }

    std::int8_t i;
};

} // namespace test
