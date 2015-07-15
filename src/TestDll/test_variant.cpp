#include "test_variant.h"

namespace {

class TestQPointer;

class PrivatePimpl {
public:
    PrivatePimpl(const char *value)
        : m_value{value}
    {}

private:
    std::string m_value;
    TestQPointer *m_qImpl;
    friend class TestQPointer;
};

class TestQPointer {
public:
    TestQPointer() noexcept = default;

    TestQPointer(const char *value)
        : m_pImpl(new PrivatePimpl{value})
    {
        m_pImpl->m_qImpl = this;
    }

    TestQPointer(const TestQPointer &other)
    {
        if (other.m_pImpl)
        {
            m_pImpl = new PrivatePimpl(*other.m_pImpl);
            m_pImpl->m_qImpl = this;
        }
    }

    TestQPointer(TestQPointer &&other) noexcept
    {
        swap(other);
    }

    TestQPointer& operator=(const TestQPointer &other)
    {
        if (m_pImpl != other.m_pImpl)
            TestQPointer{other}.swap(*this);
        return *this;
    }

    TestQPointer& operator=(TestQPointer &&other) noexcept
    {
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


    ~TestQPointer()
    {
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

}

namespace std {
template<>
inline void swap(TestQPointer &lhs, TestQPointer &rhs) noexcept
{
    lhs.swap(rhs);
}
} //std


void test_variant_1()
{
    auto q1 = TestQPointer{"Hello, World"};
    auto q2 = std::move(q1);
    q2.check();

    rtti::variant v1 = TestQPointer{"Hello, World"};
    rtti::variant v2 = std::move(v1);
    if (v1)
        v1.value<TestQPointer>().check();
    if (v2)
        v2.value<TestQPointer>().check();
}
