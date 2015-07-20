#include "test_cast.h"

#include <debug.h>

#include <memory>

namespace {


struct A
{
    int a = 1;
    virtual ~A() = default;
    DECLARE_METATYPE
};

struct B: A
{
    int b = 2;
    DECLARE_METATYPE
};


struct C: B
{
    int c = 3;
    DECLARE_METATYPE
};

struct D
{
    int d = 4;
    DECLARE_METATYPE
};

struct E: C, D
{
    int e = 5;
    DECLARE_METATYPE
};

struct VB1: virtual B
{
    int vb1 = 10;
    DECLARE_METATYPE
};

struct VB2: virtual B
{
    int vb2 = 20;
    DECLARE_METATYPE
};


struct VC: VB1, VB2
{
    int vc = 100;
    DECLARE_METATYPE
};

void register_classes()
{
    using namespace rtti;
    global_define()
        ._namespace("anonimous_1")
            ._class<A>("A")._end()
            ._class<B>("B")._base<A>()._end()
            ._class<C>("C")._base<B>()._end()
            ._class<D>("D")._end()
            ._class<E>("E")._base<C, D>()._end()
            ._class<VB1>("VB1")._base<B>()._end()
            ._class<VB2>("VB2")._base<B>()._end()
            ._class<VC>("VC")._base<VB1, VB2>()._end()
        ._end()
    ;
}

} // namespace


void test_cast_1()
{
    using namespace std;

    register_classes();

    {
        using namespace rtti;
        auto a = unique_ptr<A>(new B());

        auto b1 = meta_cast<B>(a.get());
        assert(b1 && b1->b == 2);
        auto &b2 = meta_cast<B>(*a);
        assert(b2.b == 2);
    }

    {
        using namespace rtti;
        auto a = unique_ptr<const A>(new B());

        auto b1 = meta_cast<B>(a.get());
        assert(b1 && b1->b == 2);
        auto &b2 = meta_cast<B>(*a);
        assert(b2.b == 2);
    }

    {
        using namespace rtti;
        auto a = unique_ptr<A>(new C());

        auto c1 = meta_cast<C>(a.get());
        assert(c1 && c1->c == 3);
        auto &c2 = meta_cast<C>(*a);
        assert(c2.c == 3);

        auto b1 = meta_cast<B>(a.get());
        assert(b1 && b1->b == 2);
        auto &b2 = meta_cast<B>(*a);
        assert(b2.b == 2);

        auto a1 = meta_cast<A>(a.get());
        assert(a1 && a1->a == 1);
        auto &a2 = meta_cast<A>(*a);
        assert(a2.a == 1);

        auto d1 = meta_cast<D>(a.get());
        assert(!d1);
        try
        {
            auto &d2 = meta_cast<D>(*a);
            (void) d2;
        }
        catch(const bad_meta_cast &e)
        {
            printf(e.what());
            printf("\n");
        }
    }

    {
        using namespace rtti;
        auto a = unique_ptr<A>(new E());

        auto e1 = meta_cast<E>(a.get());
        assert(e1 && e1->e == 5);
        auto &e2 = meta_cast<E>(*a);
        assert(e2.e == 5);

        auto d1 = meta_cast<D>(a.get());
        assert(d1 && d1->d == 4);
        auto &d2 = meta_cast<D>(*a);
        assert(d2.d == 4);

        auto c1 = meta_cast<C>(a.get());
        assert(c1 && c1->c == 3);
        auto &c2 = meta_cast<C>(*a);
        assert(c2.c == 3);

        auto b1 = meta_cast<B>(a.get());
        assert(b1 && b1->b == 2);
        auto &b2 = meta_cast<B>(*a);
        assert(b2.b == 2);

        auto a1 = meta_cast<A>(a.get());
        assert(a1 && a1->a == 1);
        auto &a2 = meta_cast<A>(*a);
        assert(a2.a == 1);
    }

    {
        using namespace rtti;
        A* a = new VC();

        auto c1 = meta_cast<VC>(a);
        c1 = a;
        assert(c1 && c1->vc == 100);
        auto &c2 = meta_cast<VC>(*a);
        assert(c2.vc == 100);

//        auto b1 = meta_cast<B>(a.get());
//        assert(b1 && b1->b == 2);
//        auto &b2 = meta_cast<B>(*a);
//        assert(b2.b == 2);

//        auto a1 = meta_cast<A>(a.get());
//        assert(a1 && a1->a == 1);
//        auto &a2 = meta_cast<A>(*a);
//        assert(a2.a == 1);

//        auto d1 = meta_cast<D>(a.get());
//        assert(!d1);
//        try
//        {
//            auto &d2 = meta_cast<D>(*a);
//        }
//        catch(const bad_meta_cast &e)
//        {
//            printf(e.what());
//            printf("\n");
//        }
    }

}



