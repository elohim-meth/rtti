#include "test_cast.h"

#include <rtti/rtti.h>
#include <debug.h>

#include <memory>

namespace {

struct A
{
    int a = 1;
    virtual ~A() = default;
    DECLARE_CLASSINFO
};

struct B: A
{
    int b = 2;
    DECLARE_CLASSINFO
};


struct C: B
{
    int c = 3;
    DECLARE_CLASSINFO
};

struct D
{
    int d = 4;
    virtual ~D() = default;
    DECLARE_CLASSINFO
};

struct E: C, D
{
    int e = 5;
    DECLARE_CLASSINFO
};

struct VB1: virtual B
{
    int vb1 = 10;
    DECLARE_CLASSINFO
};

struct VB2: virtual B
{
    int vb2 = 20;
    DECLARE_CLASSINFO
};


struct VC: VB1, VB2
{
    int vc = 100;
    DECLARE_CLASSINFO
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
            //should throw exception
            auto &d2 = meta_cast<D>(*a);
            (void) d2;
            assert(false);
        }
        catch(const bad_meta_cast &e)
        {}
    }

    {
        using namespace rtti;
        auto d = unique_ptr<D>(new E());

        auto e1 = meta_cast<E>(d.get());
        assert(e1 && e1->e == 5);
        auto &e2 = meta_cast<E>(*d);
        assert(e2.e == 5);

        auto d1 = meta_cast<D>(d.get());
        assert(d1 && d1->d == 4);
        auto &d2 = meta_cast<D>(*d);
        assert(d2.d == 4);

        auto c1 = meta_cast<C>(d.get());
        assert(c1 && c1->c == 3);
        auto &c2 = meta_cast<C>(*d);
        assert(c2.c == 3);

        auto b1 = meta_cast<B>(d.get());
        assert(b1 && b1->b == 2);
        auto &b2 = meta_cast<B>(*d);
        assert(b2.b == 2);

        auto a1 = meta_cast<A>(d.get());
        assert(a1 && a1->a == 1);
        auto &a2 = meta_cast<A>(*d);
        assert(a2.a == 1);
    }

    {
        using namespace rtti;
        auto a = unique_ptr<A>(new VC());

        auto c1 = meta_cast<VC>(a.get());
        assert(c1 && c1->vc == 100);
        auto &c2 = meta_cast<VC>(*a);
        assert(c2.vc == 100);

        auto vb1 = meta_cast<VB1>(a.get());
        assert(vb1 && vb1->vb1 == 10);
        auto &vb11 = meta_cast<VB1>(*a);
        assert(vb11.vb1 == 10);

        auto vb2 = meta_cast<VB2>(a.get());
        assert(vb2 && vb2->vb2 == 20);
        auto &vb21 = meta_cast<VB2>(*a);
        assert(vb21.vb2 == 20);

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
        A *a = new VC();
        variant v = a;
        auto &b = v.value<B*>();
    }

}



