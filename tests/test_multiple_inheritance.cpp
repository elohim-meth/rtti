﻿#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>
#include <rtti/metadefine.h>

namespace test {

struct BaseZ
{
    DECLARE_CLASSINFO
public:
    int m_basez = 128;
};

struct BaseY
{
    DECLARE_CLASSINFO
public:
    int m_basey = 256;
};

struct BaseX
{
    DECLARE_CLASSINFO
public:
    int m_basex = 1024;
};


struct DerivedZ1: BaseZ
{
    DECLARE_CLASSINFO
public:
    double m_derivedz = 2.18;
};

struct DerivedY1: BaseY
{
    DECLARE_CLASSINFO
public:
    double m_derivedy = 3.14;
};

struct DerivedX1: BaseX
{
    DECLARE_CLASSINFO
public:
    double m_derivedx = 1.618;
};

struct DerivedZY2: DerivedZ1, DerivedY1
{
    DECLARE_CLASSINFO
public:
    std::string m_derivedzy2 = "Hello, World!";
};

struct DerivedZY2X3: DerivedZY2, DerivedX1
{
    bool m_derivedzy2x3 = true;
};

} // namespace test

RTTI_REGISTER
{
    rtti::global_define()
        ._namespace("test")
            ._class<test::BaseZ>("BaseZ")
            ._end()
            ._class<test::BaseY>("BaseY")
            ._end()
            ._class<test::BaseX>("BaseX")
            ._end()
            ._class<test::DerivedZ1>("DerivedZ1")
                ._base<test::BaseZ>()
            ._end()
            ._class<test::DerivedY1>("DerivedY1")
                ._base<test::BaseY>()
            ._end()
            ._class<test::DerivedX1>("DerivedX1")
                ._base<test::BaseX>()
            ._end()
            ._class<test::DerivedZY2>("DerivedZY2")
                ._base<test::DerivedZ1, test::DerivedY1>()
            ._end()
            ._class<test::DerivedZY2X3>("DerivedZY2X3")
                ._base<test::DerivedZY2, test::DerivedX1>()
            ._end()
        ._end();
}
