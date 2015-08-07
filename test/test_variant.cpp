#include "test_variant.h"

#include <debug.h>

namespace {

class TestQPointer;

class PrivatePimpl {
public:
    PrivatePimpl() = delete;
    PrivatePimpl(const PrivatePimpl &other)
        : m_value{other.m_value}
    { PRINT_PRETTY_FUNC }
    PrivatePimpl& operator=(const PrivatePimpl&) = delete;
    PrivatePimpl(PrivatePimpl &&) = delete;
    PrivatePimpl& operator=(PrivatePimpl&&) = delete;
    explicit PrivatePimpl(const char *value)
        : m_value{value}
    { PRINT_PRETTY_FUNC }

private:
    std::string m_value;
    TestQPointer *m_qImpl = nullptr;
    friend class TestQPointer;
};

class TestQPointer {
public:
    TestQPointer()
    { PRINT_PRETTY_FUNC }

    explicit TestQPointer(const char *value)
        : m_pImpl{new PrivatePimpl{value}}
    {
        PRINT_PRETTY_FUNC
        m_pImpl->m_qImpl = this;
    }

    TestQPointer(const TestQPointer &other)
        : m_pImpl{other.m_pImpl ? new PrivatePimpl(*other.m_pImpl) : nullptr}
    {
        PRINT_PRETTY_FUNC
        if (m_pImpl)
            m_pImpl->m_qImpl = this;
    }

    TestQPointer(TestQPointer &&other) noexcept
    {
        PRINT_PRETTY_FUNC
        swap(other);
    }

    TestQPointer& operator=(const TestQPointer &other)
    {
        PRINT_PRETTY_FUNC
        if (m_pImpl != other.m_pImpl)
            TestQPointer{other}.swap(*this);
        return *this;
    }

    TestQPointer& operator=(TestQPointer &&other) noexcept
    {
        PRINT_PRETTY_FUNC
        if (m_pImpl != other.m_pImpl)
            TestQPointer{std::move(other)}.swap(*this);
        return *this;
    }

    void swap(TestQPointer &other) noexcept
    {
        std::swap(m_pImpl, other.m_pImpl);
        if (m_pImpl)
            m_pImpl->m_qImpl = this;
        if (other.m_pImpl)
            other.m_pImpl->m_qImpl = &other;
    }


    virtual ~TestQPointer()
    {
        PRINT_PRETTY_FUNC
        if (m_pImpl)
            delete m_pImpl;
    }

    void check() const noexcept
    {
        if (m_pImpl)
        {
            assert(this == m_pImpl->m_qImpl);
            if (this != m_pImpl->m_qImpl)
                throw std::exception{};
        }
    }

    const std::string& value() const
    {
        PRINT_PRETTY_FUNC
        assert(m_pImpl);
        return m_pImpl->m_value;
    }

    std::string& value()
    {
        PRINT_PRETTY_FUNC
        assert(m_pImpl);
        return m_pImpl->m_value;
    }

    bool empty() const
    {  return !m_pImpl; }

private:
    PrivatePimpl *m_pImpl = nullptr;

    friend class PrivatePimpl;
};

class A
{
    // This macro declares and defines one virtual method named classInfo.
    // It's needed only if A has derived classes and you need to meta_cast
    // between them or polymorphic variant behaviour for this class hierarchy.
    DECLARE_CLASSINFO
public:
    A() {}
    explicit A(int value)
        : a(value)
    {}
    A(const A &other)
        : c(other.c), a(other.a), b(other.b)
    {}
    A(A &&other) noexcept
    {
        swap(other);
    }
    A& operator=(const A &other)
    {
        if (this != &other)
            A{other}.swap(*this);
        return *this;
    }
    A& operator=(A &&other) noexcept
    {
        if (this != &other)
            A{std::move(other)}.swap(*this);
        return *this;
    }
    void swap(A &other) noexcept
    {
        std::swap(a, other.a);
        std::swap(b, other.b);
        std::swap(c, other.c);
    }
    virtual ~A()
    {
        a = b = c = -1;
    }
    virtual void print() const
    {
        std::printf("a = %d, b = %d, c = %d \n", a, b, c);
    }
    int getA() const
    {
        return a;
    }
    void setA(int value)
    {
        a = value;
    }
    int& bValue()
    {
        return b;
    }
    const int& bValue() const
    {
        return b;
    }

    int c = -1;
private:
    int a = -1;
    int b = -1;
};

class B: public A
{
    DECLARE_CLASSINFO
public:
    B() : A()
    { PRINT_PRETTY_FUNC }

    explicit B(int value)
        : A{value}, d{value}
    {
        PRINT_PRETTY_FUNC
    }

    B(const B &other)
        : A{other}, d{other.d}
    {
        PRINT_PRETTY_FUNC
    }

    B(B &&other) noexcept
        : A{std::move(other)}
    {
        PRINT_PRETTY_FUNC
        std::swap(d, other.d);
    }

    B& operator=(const B &other)
    {
        PRINT_PRETTY_FUNC
        if (this != &other)
            B{other}.swap(*this);
        return *this;
    }

    B& operator=(B &&other) noexcept
    {
        PRINT_PRETTY_FUNC
        if (this != &other)
            B{std::move(other)}.swap(*this);
        return *this;
    }

    void swap(B &other) noexcept
    {
        A::swap(other);
        std::swap(d, other.d);
    }


    virtual ~B()
    {
        PRINT_PRETTY_FUNC
        d = -1;
    }

    void print() const override
    {
        PRINT_PRETTY_FUNC
        A::print();
        std::printf("d = %d\n", d);
    }

    int getD() const
    {
        PRINT_PRETTY_FUNC
        return d;
    }

    void setD(int value)
    {
        PRINT_PRETTY_FUNC
        d = value;
    }

private:
    int d = -1;
};


void register_classes()
{
    using namespace rtti;
    global_define()
        ._namespace("anonimous_2")
            ._class<A>("A")
                ._constructor<int>()
                ._method("print", &A::print)
                ._property("a", &A::getA, &A::setA)
                ._method<int& (A::*)()>("bValue", &A::bValue)
                ._method<const int& (A::*)() const>("bValue", &A::bValue)
                ._property("c", &A::c)
            ._end()
            ._class<B>("B")._base<A>()
                ._property("d", &B::getD, &B::setD)
            ._end()
            ._class<TestQPointer>("TestQPointer")
                ._constructor<const char*>()
                ._method("empty", &TestQPointer::empty)
                ._method("check", &TestQPointer::check)
                ._method<std::string&(TestQPointer::*)()>("value", &TestQPointer::value)
                ._method<const std::string&(TestQPointer::*)() const>("value", &TestQPointer::value)
            ._end()
        ._end()
    ;
}

} // namespace

namespace std {
template<>
inline void swap(TestQPointer &lhs, TestQPointer &rhs) noexcept
{
    lhs.swap(rhs);
}
} //std


void test_variant_1()
{
    {
        auto q1 = TestQPointer{"Hello, World"};
        auto q2 = TestQPointer{"qwerty"};
        q1 = std::move(q2);
        q1.check();
        q2.check();
    }

    std::printf("\n");

    {
        rtti::variant v1 = TestQPointer{"Hello, World"};
        rtti::variant v2 = TestQPointer{"qwerty"};
        v1 = std::move(v2);
        assert(v1);
        v1.value<TestQPointer>().check();
        assert(!v2);
    }

    std::printf("\n");

    {
        auto lambda = []() { return rtti::variant{TestQPointer{"123456"}}; };
        auto q1 = lambda().value<TestQPointer>();
        q1.check();
    }

    register_classes();

    std::printf("\n");

    {
        rtti::variant v3 = "Hello, World";
        auto q3 = v3.to<TestQPointer>();
    }

    std::printf("\n");

    {
        using namespace rtti;
        auto q = new TestQPointer{"111"};
        variant v = std::ref(q);

        auto c = MetaClass::findByTypeId(metaTypeId<TestQPointer>()); assert(c);
        auto m = c->getMethod<const TestQPointer&>("value"); assert(m);
        auto s = m->invoke(v);
        try {
            s.value<std::string>() = "222"; // shoud throw
            assert(false);
        } catch (const bad_variant_cast &e) { LOG_RED(e.what()); }

        m = c->getMethod<TestQPointer&>("value"); assert(m);
        s = m->invoke(v);
        s.value<std::string>() = "222";
        assert(q->value() == "222");

        const auto *q1 = q;
        variant v1 = std::ref(q1);
        try {
            s = m->invoke(v1); //should throw
            assert(false);
        } catch (const bad_variant_cast &e) { LOG_RED(e.what()); }

        variant v2 = q;
        s = m->invoke(v2); //shouldn't throw

        variant v3 = q1;
        try {
            s = m->invoke(v3); //should throw
            assert(false);
        } catch (const bad_variant_cast &e) { LOG_RED(e.what()); }
        delete q;
    }

    std::printf("\n");

    {
        rtti::variant v = B{100};
        assert(v.is<B>() && v.is<const B>());
        assert(v.is<A>() && v.is<const A>());
        assert(!v.is<B*>() && !v.is<const B*>());
        assert(!v.is<A*>() && !v.is<const A*>());
        assert(!v.is<int>());
    }

    std::printf("\n");

    {
        rtti::variant v = new B{100};
        assert(v.is<B*>() && v.is<const B*>());
        assert(v.is<A*>() && v.is<const A*>());
        assert(!v.is<B>() && !v.is<const B>());
        assert(!v.is<A>() && !v.is<const A>());
        assert(!v.is<int*>() && !v.is<const int*>());
        delete v.value<B*>();
    }

    std::printf("\n");

    {
        const A *a = new B{100};
        rtti::variant v = a;
        assert(!v.is<B*>() && v.is<const B*>());
        assert(!v.is<A*>() && v.is<const A*>());
        assert(!v.is<B>() && !v.is<const B>());
        assert(!v.is<A>() && !v.is<const A>());
        assert(!v.is<int*>() && !v.is<const int*>());
        delete a;
    }

    {
        const B b{100};
        rtti::variant v = b;
        assert(!v.is<B*>() && !v.is<const B*>());
        assert(!v.is<A*>() && !v.is<const A*>());
        assert(v.is<B>() && v.is<const B>());
        assert(v.is<A>() && v.is<const A>());
        assert(!v.is<int*>() && !v.is<const int*>());
    }

    {
        const B b{100};
        rtti::variant v = std::ref(b);
        assert(!v.is<B*>() && !v.is<const B*>());
        assert(!v.is<A*>() && !v.is<const A*>());
        assert(v.is<B>() && !v.is<B&>() && v.is<const B>() && v.is<const B&>());
        assert(v.is<A>() && !v.is<A&>() && v.is<const A>() && v.is<const A&>());
        assert(!v.is<int*>() && !v.is<int&>() && !v.is<const int*>() && !v.is<const int&>());
    }

    auto lambda = [] (const rtti::variant &v)
    {
        auto MC = rtti::MetaClass::findByTypeId(v.classInfo().typeId); assert(MC);

        auto print = MC->getMethod("print"); assert(print);
        print->invoke(v);

        auto aP = MC->getProperty("a"); assert(aP);
        auto r = aP->get(v); assert(r.to<int>() == 100);

        r = 256;
        aP->set(v, r);
        r = aP->get(v); assert(r.value<int>() == 256);

        print->invoke(v);

        {
            auto bvalM = MC->getMethod<A&>("bValue"); assert(bvalM);
            auto r = bvalM->invoke(v); assert(r.value<int>() == -1);
            r.value<int>() = 128;
        }

        {
            auto bvalCM = MC->getMethod<const A&>("bValue"); assert(bvalCM);
            auto r = bvalCM->invoke(v); assert(r.to<std::string>() == "128");
        }

        print->invoke(v);

        {
            auto dP = MC->getProperty("d");
            if (dP)
            {
                auto r = dP->get(v);
                dP->set(v, 1024);

            }
        }

    };

    std::printf("\n");

    {
        rtti::variant v = A{100};
        lambda(v);
        auto a = new A{100};
        v = a;
        lambda(v);
        delete a;
    }

    std::printf("\n");

    {
        rtti::variant v = B{100};
        lambda(v);
        auto b = new B{100};
        v = b;
        lambda(v);
        delete b;
    }

    std::printf("\n");

    {
        B b{100};
        rtti::variant v = std::ref(b);
        lambda(v);
    }

    std::printf("\n");

    {
        using namespace rtti;
        auto qpMC = MetaNamespace::global()->getNamespace("anonimous_2")->getClass("TestQPointer"); assert(qpMC);
        auto empM = qpMC->getMethod("empty"); assert(empM);
        auto valCM = qpMC->getMethod<const TestQPointer&>("value"); assert(valCM);
        auto chkM = qpMC->getMethod("check"); assert(chkM);

        auto defC = qpMC->defaultConstructor(); assert(defC);
        try {
            auto obj = defC->invoke(0); assert(false);
        } catch (const invoke_error &e) { LOG_RED(e.what()); };
        auto obj = defC->invoke(); assert(obj);
        chkM->invoke(obj);
        assert(empM->invoke(obj).to<bool>());

        auto cusC = qpMC->getConstructor<const char*>(); assert(cusC);
        obj = cusC->invoke("Hello, World");
        chkM->invoke(obj);
        assert(!empM->invoke(obj).to<bool>());
        assert(valCM->invoke(obj).cvalue<std::string>() == "Hello, World");

        auto copyC = qpMC->copyConstructor(); assert(copyC);
        try {
            auto copy = copyC->invoke("1234"); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); };
        auto copy = copyC->invoke(obj); assert(copy);
        chkM->invoke(obj); chkM->invoke(copy);
        assert(!empM->invoke(copy).to<bool>());
        assert(valCM->invoke(copy).cvalue<std::string>() == "Hello, World");

        auto moveC = qpMC->moveConstructor(); assert(moveC);
        try {
            auto move = moveC->invoke(copy); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); };
        auto move = moveC->invoke(std::move(copy));
        chkM->invoke(move);
        assert(empM->invoke(copy).to<bool>());
        assert(!empM->invoke(move).to<bool>());
        assert(valCM->invoke(move).cvalue<std::string>() == "Hello, World");

    }

    std::printf("\n");
}
