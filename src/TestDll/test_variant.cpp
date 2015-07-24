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

    void check() const
    {
        if (m_pImpl)
            assert(this == m_pImpl->m_qImpl);
    }

private:
    PrivatePimpl *m_pImpl = nullptr;

    friend class PrivatePimpl;
};

class A
{
    DECLARE_CLASSINFO
public:
    A()
    { PRINT_PRETTY_FUNC }

    explicit A(int value)
        : a(value)
    {
        PRINT_PRETTY_FUNC
    }

    A(const A &other)
        : a(other.a)
    {
        PRINT_PRETTY_FUNC
    }

    A(A &&other) noexcept
    {
        PRINT_PRETTY_FUNC
        std::swap(a, other.a);
    }

    A& operator=(const A &other)
    {
        PRINT_PRETTY_FUNC
        if (this != &other)
            A{other}.swap(*this);
        return *this;
    }

    A& operator=(A &&other) noexcept
    {
        PRINT_PRETTY_FUNC
        if (this != &other)
            A{std::move(other)}.swap(*this);
        return *this;
    }

    void swap(A &other) noexcept
    {
        std::swap(a, other.a);
    }


    virtual ~A() noexcept
    {
        PRINT_PRETTY_FUNC
        a = -1;
    }

    virtual void print() const
    {
        std::printf("a = %d\n", a);
    }

private:
    int a = -1;
};

class B: public A
{
    DECLARE_CLASSINFO
public:
    B() : A()
    { PRINT_PRETTY_FUNC }

    explicit B(int value)
        : A{value}, b{value}
    {
        PRINT_PRETTY_FUNC
    }

    B(const B &other)
        : A{other}, b{other.b}
    {
        PRINT_PRETTY_FUNC
    }

    B(B &&other) noexcept
        : A{std::move(other)}
    {
        PRINT_PRETTY_FUNC
        std::swap(b, other.b);
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
        std::swap(b, other.b);
    }


    virtual ~B() noexcept
    {
        PRINT_PRETTY_FUNC
        b = -1;
    }

    void print() const override
    {
        A::print();
        std::printf("b = %d\n", b);
    }

private:
    int b = -1;
};


void register_classes()
{
    using namespace rtti;
    global_define()
        ._namespace("anonimous_2")
            ._class<A>("A")
                ._method("print", &A::print)
            ._end()
            ._class<B>("B")._base<A>()._end()
            ._class<TestQPointer>("TestQPointer")
                ._constructor<const char*>()
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
        rtti::variant v = B{100};
        assert(v.is<B>() && v.is<const B>());
        assert(v.is<A>() && v.is<const A>());
        assert(!v.is<B*>() && !v.is<const B*>());
        assert(!v.is<A*>() && !v.is<const A*>());
        v.value<A>().print();
        assert(!v.is<int>());
    }

    std::printf("\n");

    {
        rtti::variant v = new B{100};
        assert(v.is<B*>() && v.is<const B*>());
        assert(v.is<A*>() && v.is<const A*>());
        assert(!v.is<B>() && !v.is<const B>());
        assert(!v.is<A>() && !v.is<const A>());
        v.value<A*>()->print();
        assert(!v.is<int*>() && !v.is<const int*>());
        delete v.value<B*>();
    }

    std::printf("\n");

    {
        const A *a = new B{100};
        rtti::variant v = a;
        assert(v.is<B*>() && v.is<const B*>());
        assert(v.is<A*>() && v.is<const A*>());
        assert(!v.is<B>() && !v.is<const B>());
        assert(!v.is<A>() && !v.is<const A>());
        v.value<B*>()->print();
        assert(!v.is<int*>() && !v.is<const int*>());

        auto c = rtti::MetaClass::findByTypeId(rtti::metaTypeId<B>());
        auto m = c->getMethod("print");
        if (m)
            m->invoke(v);
        delete a;
    }

    std::printf("\n");
}
