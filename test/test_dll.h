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


class Test1 {};
class Test2
{
public:
    virtual ~Test2() noexcept = default;
};
class Test3
{
public:
    virtual ~Test3() noexcept {}
};
class Test4
{
public:
    Test4() = default;
    Test4(Test4 const&) = delete;
    Test4& operator= (Test4 const&) = delete;
    Test4(Test4 &&) = delete;
    Test4& operator= (Test4 &&) = delete;
};
class Test5
{
public:
    Test5() = default;
    Test5(Test5 const&) = default;
    Test5& operator= (Test5 const&) = default;
    Test5(Test5 &&) = delete;
    Test5& operator= (Test5 &&) = delete;
};
class Test6
{
public:
    Test6() = default;
    Test6(Test6 const&) {}
    Test6& operator= (Test6 const&) { return *this; }
};
class Test7
{
public:
    Test7() = default;
    Test7(Test7 const&) = default;
    Test7& operator= (Test7 const&) = default;
};
class Test8
{
public:
    Test8() = default;
    Test8(Test8 &&) {}
    Test8& operator= (Test8 &&) { return *this; }
};
class Test9
{
public:
    Test9() = default;
    Test9(Test9 &&) = default;
    Test9& operator= (Test9 &&) = default;
};
class Test10
{
public:
    Test10() = default;
    Test10(Test10 &&) = delete;
    Test10& operator= (Test10 &&) = delete;
};

class CopyAndMove
{
public:
    CopyAndMove() = default;
    CopyAndMove(CopyAndMove const&) = default;
    CopyAndMove& operator= (CopyAndMove const&) = default;
    CopyAndMove(CopyAndMove &&) = default;
    CopyAndMove& operator= (CopyAndMove &&) = default;

private:
    std::vector<std::size_t> m_v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
};

class CopyOnly
{
public:
    CopyOnly() = default;
    CopyOnly(CopyOnly const&) = default;
    CopyOnly& operator= (CopyOnly const&) = default;

private:
    std::vector<std::size_t> m_v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
};

class MoveOnly
{
public:
    MoveOnly() = default;
    MoveOnly(MoveOnly &&) = default;
    MoveOnly& operator= (MoveOnly &&) = default;

private:
    std::vector<std::size_t> m_v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
};



} // namespace test
