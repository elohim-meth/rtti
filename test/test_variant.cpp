#include "test_variant.h"

#include <finally.h>
#include <debug.h>

namespace {

class TestQPointer;

class PrivatePimpl {
public:
    PrivatePimpl() = delete;
    PrivatePimpl(const PrivatePimpl &other)
        : m_value{other.m_value}
    { PRINT_PRETTY_FUNC; }
    PrivatePimpl& operator=(const PrivatePimpl&) = delete;
    //PrivatePimpl(PrivatePimpl &&) = delete;
    PrivatePimpl& operator=(PrivatePimpl&&) = delete;
    explicit PrivatePimpl(const char *value)
        : m_value{value}
    { PRINT_PRETTY_FUNC; }

private:
    std::string m_value;
    TestQPointer *m_qImpl = nullptr;

    friend class TestQPointer;
    friend void register_classes();
};

class TestQPointer {
public:
    TestQPointer()
    { PRINT_PRETTY_FUNC; }

    //explicit TestQPointer(const char *value)
    TestQPointer(const char *value)
        : m_pImpl{new PrivatePimpl{value}}
    {
        PRINT_PRETTY_FUNC;
        m_pImpl->m_qImpl = this;
    }

    TestQPointer(const TestQPointer &other)
        : m_pImpl{other.m_pImpl ? new PrivatePimpl(*other.m_pImpl) : nullptr}
    {
        PRINT_PRETTY_FUNC;
        if (m_pImpl)
            m_pImpl->m_qImpl = this;
    }

    TestQPointer(TestQPointer &&other) noexcept
    {
        PRINT_PRETTY_FUNC;
        swap(other);
    }

    TestQPointer& operator=(const TestQPointer &other)
    {
        PRINT_PRETTY_FUNC;
        if (m_pImpl != other.m_pImpl)
            TestQPointer{other}.swap(*this);
        return *this;
    }

    TestQPointer& operator=(TestQPointer &&other) noexcept
    {
        PRINT_PRETTY_FUNC;
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
        PRINT_PRETTY_FUNC;
        if (m_pImpl)
            delete m_pImpl;
    }

    bool check() const
    {
        auto result = true;
        if (m_pImpl)
            result = (this == m_pImpl->m_qImpl);
        assert(result);
        return result;
    }

    const std::string& value() const
    {
        PRINT_PRETTY_FUNC;
        assert(m_pImpl);
        return m_pImpl->m_value;
    }

    std::string& value()
    {
        PRINT_PRETTY_FUNC;
        assert(m_pImpl);
        return m_pImpl->m_value;
    }

    bool empty() const
    {  return !m_pImpl; }

private:
    PrivatePimpl *m_pImpl = nullptr;

    friend class PrivatePimpl;
    friend void register_classes();
};

class TestA
{
    // This macro declares and defines one virtual method named classInfo.
    // It's needed only if A has derived classes and you need to meta_cast
    // between them or polymorphic variant behaviour for this class hierarchy.
    DECLARE_CLASSINFO
public:
    TestA() {}
    explicit TestA(int value)
        : a(value)
    {}
    TestA(const TestA &other)
        : c(other.c), a(other.a), b(other.b)
    {}
    TestA(TestA &&other) noexcept
    {
        swap(other);
    }
    TestA& operator=(const TestA &other)
    {
        if (this != &other)
            TestA{other}.swap(*this);
        return *this;
    }
    TestA& operator=(TestA &&other) noexcept
    {
        if (this != &other)
            TestA{std::move(other)}.swap(*this);
        return *this;
    }
    void swap(TestA &other) noexcept
    {
        std::swap(a, other.a);
        std::swap(b, other.b);
        std::swap(c, other.c);
    }
    virtual ~TestA()
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

class TestB: public TestA
{
    DECLARE_CLASSINFO
public:
    TestB() : TestA()
    { PRINT_PRETTY_FUNC; }

    explicit TestB(int value)
        : TestA{value}, d{value}
    {
        PRINT_PRETTY_FUNC;
    }

    TestB(const TestB &other)
        : TestA{other}, d{other.d}
    {
        PRINT_PRETTY_FUNC;
    }

    TestB(TestB &&other) noexcept
        : TestA{std::move(other)}
    {
        PRINT_PRETTY_FUNC;
        std::swap(d, other.d);
    }

    TestB& operator=(const TestB &other)
    {
        PRINT_PRETTY_FUNC;
        if (this != &other)
            TestB{other}.swap(*this);
        return *this;
    }

    TestB& operator=(TestB &&other) noexcept
    {
        PRINT_PRETTY_FUNC;
        if (this != &other)
            TestB{std::move(other)}.swap(*this);
        return *this;
    }

    void swap(TestB &other) noexcept
    {
        TestA::swap(other);
        std::swap(d, other.d);
    }


    virtual ~TestB()
    {
        PRINT_PRETTY_FUNC;
        d = -1;
    }

    void print() const override
    {
        PRINT_PRETTY_FUNC;
        TestA::print();
        std::printf("d = %d\n", d);
    }

    int getD() const
    {
        PRINT_PRETTY_FUNC;
        return d;
    }

    void setD(int value)
    {
        PRINT_PRETTY_FUNC;
        d = value;
    }

private:
    int d = -1;
};


void register_classes()
{
    using namespace rtti;
    global_define()
        ._namespace("anonimous")
            ._class<TestA>("TestA")
                ._constructor<int>()
                ._method("print", &TestA::print)
                ._property("a", &TestA::getA, &TestA::setA)
                ._method<int& (TestA::*)()>("bValue", &TestA::bValue)
                ._method<const int& (TestA::*)() const>("bValue", &TestA::bValue)
                ._property("c", &TestA::c)
            ._end()
            ._class<TestB>("TestB")._base<TestA>()
                ._property("d", &TestB::getD, &TestB::setD)
            ._end()
            ._class<PrivatePimpl>("PrivatePimpl")
                ._constructor<char const*>()
                ._property("m_pImpl", &PrivatePimpl::m_qImpl)
                ._property("m_value", &PrivatePimpl::m_value)
            ._end()
            ._class<TestQPointer>("TestQPointer")
                ._constructor<char const*>()
                ._method("empty", &TestQPointer::empty)
                ._method("check", &TestQPointer::check)
                ._method<std::string& (TestQPointer::*)()>("value", &TestQPointer::value)
                ._method<std::string const& (TestQPointer::*)() const>("value", &TestQPointer::value)
                ._property("m_pImpl", &TestQPointer::m_pImpl)
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
        q2 = TestQPointer{"123456"};
        q1 = std::move(q2);
        q1.check();
        q2.check();
    }

    std::printf("\n");

    {
        std::printf("Size of rtti::variant = %llu\n", sizeof(rtti::variant));
        rtti::variant v1 = TestQPointer{"asdfgh"};
        rtti::variant v2 = TestQPointer{"qwerty"};
        v2 = TestQPointer{"123456"};
        v1 = std::move(v2);
        assert(v1);
        v1.value<TestQPointer>().check();
        assert(!v2);
    }

    std::printf("\n");

    {
        auto lambda = []() { return rtti::variant{TestQPointer{"123456"}}; };
        auto q = lambda().value<TestQPointer>();
        q.check();
    }

    register_classes();

    std::printf("\n");

    {
        rtti::variant v = "Hello, World";
        auto q1 = v.to<TestQPointer>();
        auto q = TestQPointer{"123456"};
        q = v.to<TestQPointer>();

        {
            v.convert<TestQPointer>();
            auto *mcQptr = v.metaClass(); assert(mcQptr);
            auto *pPImpl = mcQptr->getProperty("m_pImpl"); assert(pPImpl);
            auto vPImpl = pPImpl->get(v); assert(!vPImpl.empty());
            auto *mcPimpl = vPImpl.metaClass(); assert(mcPimpl);
            auto *pPImplValue = mcPimpl->getProperty("m_value"); assert(pPImplValue);
            auto vPImplValue = pPImplValue->get(vPImpl);
            assert(vPImplValue.cvalue<std::string>() == "Hello, World");
            vPImplValue.metaClass()->forceDeferredDefine();
            pPImplValue->set(vPImpl, "Foo - Bar");
            assert(vPImplValue.cvalue<std::string>() == "Foo - Bar");

        }
    }

    std::printf("\n");

    {
        using namespace rtti;
        auto q = new TestQPointer{"111"};
        variant v = std::ref(q);

        auto mcQptr = MetaClass::findByTypeId(metaTypeId<TestQPointer>()); assert(mcQptr);
        auto mValue = mcQptr->getMethod<const TestQPointer&>("value"); assert(mValue);
        auto s = mValue->invoke(v);
        try {
            s.value<std::string>() = "222"; // shoud throw
            assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }

        mValue = mcQptr->getMethod<TestQPointer&>("value"); assert(mValue);
        s = mValue->invoke(v);
        s.value<std::string>() = "222";
        assert(q->value() == "222");

        const auto *q1 = q;
        variant v1 = std::ref(q1);
        try {
            s = mValue->invoke(v1); //should throw
            assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }

        variant v2 = q;
        s = mValue->invoke(v2); //shouldn't throw

        variant v3 = q1;
        try {
            s = mValue->invoke(v3); //should throw
            assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        delete q;
    }

    std::printf("\n");

    {
        rtti::variant v = TestB{100};
        assert(v.is<TestB>() && v.is<const TestB>());
        assert(v.is<TestA>() && v.is<const TestA>());
        assert(!v.is<TestB*>() && !v.is<const TestB*>());
        assert(!v.is<TestA*>() && !v.is<const TestA*>());
        assert(!v.is<int>());
    }

    std::printf("\n");

    {
        rtti::variant v = new TestB{100};
        assert(v.is<TestB*>() && v.is<const TestB*>());
        assert(v.is<TestA*>() && v.is<const TestA*>());
        assert(!v.is<TestB>() && !v.is<const TestB>());
        assert(!v.is<TestA>() && !v.is<const TestA>());
        assert(!v.is<int*>() && !v.is<const int*>());
        delete v.value<TestB*>();
    }

    std::printf("\n");

    {
        const TestA *a = new TestB{100};
        rtti::variant v = a;
        assert(!v.is<TestB*>() && v.is<const TestB*>());
        assert(!v.is<TestA*>() && v.is<const TestA*>());
        assert(!v.is<TestB>() && !v.is<const TestB>());
        assert(!v.is<TestA>() && !v.is<const TestA>());
        assert(!v.is<int*>() && !v.is<const int*>());
        delete a;
    }

    {
        const TestB b{100};
        rtti::variant v = b;
        assert(!v.is<TestB*>() && !v.is<const TestB*>());
        assert(!v.is<TestA*>() && !v.is<const TestA*>());
        assert(v.is<TestB>() && v.is<const TestB>());
        assert(v.is<TestA>() && v.is<const TestA>());
        assert(!v.is<int*>() && !v.is<const int*>());
    }

    {
        const TestB b{100};
        rtti::variant v = std::ref(b);
        assert(!v.is<TestB*>() && !v.is<const TestB*>());
        assert(!v.is<TestA*>() && !v.is<const TestA*>());
        assert(v.is<TestB>() && !v.is<TestB&>() && v.is<const TestB>() && v.is<const TestB&>());
        assert(v.is<TestA>() && !v.is<TestA&>() && v.is<const TestA>() && v.is<const TestA&>());
        assert(!v.is<int*>() && !v.is<int&>() && !v.is<const int*>() && !v.is<const int&>());
    }

    auto lambda = [] (rtti::variant &v)
    {
        auto MC = v.metaClass(); assert(MC);

        auto print = MC->getMethod("print"); assert(print);
        print->invoke(v);

        auto aP = MC->getProperty("a"); assert(aP);
        auto r = aP->get(v); assert(r.to<int>() == 100);

        r = 256;
        aP->set(v, r);
        r = aP->get(v); assert(r.value<int>() == 256);

        print->invoke(v);

        {
            auto bvalM = MC->getMethod<TestA&>("bValue"); assert(bvalM);
            auto r = bvalM->invoke(v); assert(r.value<int>() == -1);
            r.value<int>() = 128;
        }

        {
            auto bvalCM = MC->getMethod<const TestA&>("bValue"); assert(bvalCM);
            auto r = bvalCM->invoke(v);
            assert(r.cvalue<int>() == 128);
            assert(r.to<std::string>() == "128");
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
        rtti::variant v = TestA{100};
        lambda(v);
        auto a = new TestA{100};
        v = a;
        lambda(v);
        delete a;
    }

    std::printf("\n");

    {
        rtti::variant v = TestB{100};
        lambda(v);
        auto b = new TestB{100};
        v = b;
        lambda(v);
        delete b;
    }

    std::printf("\n");

    {
        TestB b{100};
        rtti::variant v = std::ref(b);
        lambda(v);
    }

    std::printf("\n");

    {
        using namespace rtti;
        auto qpMC = MetaNamespace::global()->getNamespace("anonimous")->getClass("TestQPointer"); assert(qpMC);
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

        auto cusC = qpMC->getConstructor<char const*>(); assert(cusC);
        obj = cusC->invoke("Hello, World");
        chkM->invoke(obj);
        assert(!empM->invoke(obj).to<bool>());
        assert(valCM->invoke(obj).cvalue<std::string>() == "Hello, World");

        auto copyC = qpMC->copyConstructor(); assert(copyC);
        auto copy = copyC->invoke("1234");
        copy = copyC->invoke(obj); assert(copy);
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

    {
        using namespace rtti;
        variant v = 1;
        assert(v.to<int>() == 1);
        assert(v.to<int const>() == 1);
        assert(v.value<int>() == 1);
        assert(v.value<int const>() == 1);
        assert(v.cvalue<int>() == 1);
        assert(v.cvalue<int const>() == 1);
    }

    {
        using namespace rtti;
        variant const v = 1;
        assert(v.to<int>() == 1);
        assert(v.to<int const>() == 1);
        assert(v.value<int>() == 1);
        assert(v.value<int const >() == 1);
        assert(v.cvalue<int>() == 1);
        assert(v.cvalue<int const>() == 1);
    }


    {
        using namespace rtti;
        int i = 1;
        variant v = std::cref(i);
        assert(v.to<int>() == 1);
        assert(v.to<int const>() == 1);
        try {
            v.value<int>(); assert(false);
        } catch (bad_cast const &e) { LOG_RED(e.what()); }
        assert(v.value<int const>() == 1);
        assert(v.cvalue<int>() == 1);
        assert(v.cvalue<int const>() == 1);
    }

    {
        using namespace rtti;
        int i = 1;
        variant const v = std::cref(i);
        assert(v.to<int>() == 1);
        assert(v.to<int const>() == 1);
        assert(v.value<int>() == 1);
        assert(v.value<int const>() == 1);
        assert(v.cvalue<int>() == 1);
        assert(v.cvalue<int const>() == 1);
    }

    {
        using namespace rtti;
        const int i = 1;
        variant v = std::ref(i);
        assert(v.to<int>() == 1);
        assert(v.to<const int>() == 1);
        try {
            v.value<int>(); assert(false);
        } catch (bad_cast const &e) { LOG_RED(e.what()); }
        assert(v.value<const int>() == 1);
        assert(v.cvalue<int>() == 1);
        assert(v.cvalue<const int>() == 1);
    }

    {
        using namespace rtti;
        const int i = 1;
        const variant v = std::ref(i);
        assert(v.to<int>() == 1);
        assert(v.to<const int>() == 1);
        assert(v.value<int>() == 1);
        assert(v.value<const int>() == 1);
        assert(v.cvalue<int>() == 1);
        assert(v.cvalue<const int>() == 1);
    }

    {
        using namespace rtti;
        int i = 1;
        variant v = &i;
        assert(*v.to<int*>() == 1);
        assert(*v.to<const int*>() == 1);
        assert(*v.value<int*>() == 1);
        try {
            v.value<const int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(*v.cvalue<int*>() == 1);
        assert(*v.cvalue<const int*>() == 1);
    }

    {
        using namespace rtti;
        int i = 1;
        const variant v = &i;
        assert(*v.to<int*>() == 1);
        assert(*v.to<const int*>() == 1);
        assert(*v.value<int*>() == 1);
        assert(*v.value<const int*>() == 1);
        assert(*v.cvalue<int*>() == 1);
        assert(*v.cvalue<const int*>() == 1);
    }

    {
        using namespace rtti;
        const int i = 1;
        variant v = &i;
        try {
            v.to<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(*v.to<const int*>() == 1);
        try {
            v.value<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(*v.value<const int*>() == 1);
        try {
            v.cvalue<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(*v.cvalue<const int*>() == 1);
    }

    {
        using namespace rtti;
        const int i = 1;
        const variant v = &i;
        try {
            v.to<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(*v.to<const int*>() == 1);
        try {
            v.value<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(*v.value<const int*>() == 1);
        try {
            v.cvalue<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(*v.cvalue<const int*>() == 1);
    }

    {
        using namespace rtti;
        int i[7] = {1, 2, 3, 4, 5, 6, 7};
        variant v = i;
        assert(v.to<int*>()[0] == 1);
        assert(v.to<const int*>()[1] == 2);
        try {
            v.value<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }

        try {
            v.value<const int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.cvalue<int*>()[2] == 3);
        assert(v.cvalue<const int*>()[3] == 4);

        assert(v.value<int[7]>()[4] == 5);
        assert(v.value<const int[7]>()[5] == 6);
        assert(v.value<int[3]>()[1] == 2);
        try {
            v.value<int[10]>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
    }

    {
        using namespace rtti;
        int i[7] = {1, 2, 3, 4, 5, 6, 7};
        variant const v = i;
        try {
            v.to<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.to<int const*>()[1] == 2);
        try {
            v.value<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.value<int const*>()[1] == 2);
        try {
            v.cvalue<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.cvalue<int const*>()[3] == 4);

        assert(v.value<int[7]>()[4] == 5);
        assert(v.value<const int[7]>()[5] == 6);
        assert(v.value<int[3]>()[1] == 2);
        try {
            v.value<int[10]>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
    }

    {
        using namespace rtti;
        int i[7] = {1, 2, 3, 4, 5, 6, 7};
        variant v = std::ref(i);
        assert(v.to<int*>()[0] == 1);
        assert(v.to<const int*>()[1] == 2);
        try {
            v.value<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }

        try {
            v.value<const int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.cvalue<int*>()[2] == 3);
        assert(v.cvalue<const int*>()[3] == 4);

        assert(v.value<int[7]>()[4] == 5);
        assert(v.value<const int[7]>()[5] == 6);
        assert(v.value<int[3]>()[1] == 2);
        try {
            v.value<int[10]>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
    }

    {
        using namespace rtti;
        int i[7] = {1, 2, 3, 4, 5, 6, 7};
        variant v = std::cref(i);
        try {
            v.to<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.to<const int*>()[1] == 2);
        try {
            v.value<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        try {
            v.value<const int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        try {
            v.cvalue<int*>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.cvalue<const int*>()[3] == 4);

        try {
            v.value<int[7]>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.value<const int[7]>()[5] == 6);
        try {
            v.value<int[3]>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        assert(v.value<int const[3]>()[1] == 2);
        try {
            v.value<int[10]>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
        try {
            v.value<int const[10]>(); assert(false);
        } catch (const bad_cast &e) { LOG_RED(e.what()); }
    }

    {
        using namespace rtti;
        int i[2][3] = {{1, 2, 3}, {4, 5, 6}};
        variant v = i;
        {
            auto t = v.to<int(*)[3]>();
            assert(t[0][0] == 1 && t[0][1] == 2 && t[0][2] == 3 &&
                   t[1][0] == 4 && t[1][1] == 5 && t[1][2] == 6);
        }
        {
            auto t = v.to<int const(*)[3]>();
            assert(t[0][0] == 1 && t[0][1] == 2 && t[0][2] == 3 &&
                   t[1][0] == 4 && t[1][1] == 5 && t[1][2] == 6);
        }
        {
            try {
                v.value<int(*)[3]>(); assert(false);
            } catch (const rtti::bad_cast &e) { LOG_RED(e.what()); }

        }
        {
            auto &t = v.cvalue<int(*)[3]>();
            assert(t[0][0] == 1 && t[0][1] == 2 && t[0][2] == 3 &&
                   t[1][0] == 4 && t[1][1] == 5 && t[1][2] == 6);
        }
        {
            try {
                v.value<int const(*)[3]>(); assert(false);
            } catch (const rtti::bad_cast &e) { LOG_RED(e.what()); }
        }
        {
            auto &t = v.cvalue<int const(*)[3]>();
            assert(t[0][0] == 1 && t[0][1] == 2 && t[0][2] == 3 &&
                   t[1][0] == 4 && t[1][1] == 5 && t[1][2] == 6);
        }
        {
            auto &t = v.value<int[2][3]>();
            assert(t[0][0] == 1 && t[0][1] == 2 && t[0][2] == 3 &&
                   t[1][0] == 4 && t[1][1] == 5 && t[1][2] == 6);
        }
        {
            auto &t = v.value<int const[2][3]>();
            assert(t[0][0] == 1 && t[0][1] == 2 && t[0][2] == 3 &&
                   t[1][0] == 4 && t[1][1] == 5 && t[1][2] == 6);
        }
    }

    {
//        int const * const * const * i;
//        int *** j;
//        i = j;
    }
}

