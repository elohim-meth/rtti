#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>
#include <rtti/metadefine.h>

namespace {

uint16_t explicit_constructed = 0;
uint16_t default_constructed = 0;
uint16_t copy_constructed = 0;
uint16_t move_constructed = 0;
uint16_t copy_assigned = 0;
uint16_t move_assigned = 0;

void reset_counters()
{
    explicit_constructed = 0;
    default_constructed = 0;
    copy_constructed = 0;
    move_constructed = 0;
    copy_assigned = 0;
    move_assigned = 0;
}

}

namespace test {

class TestQPointer;

class PrivateImpl {
public:
    PrivateImpl() = default;
    PrivateImpl(const PrivateImpl &other)
        : m_value{other.m_value}
        , m_priority{other.m_priority}
    {}
    PrivateImpl(PrivateImpl &&other) = delete;
    PrivateImpl& operator=(const PrivateImpl&) = delete;
    PrivateImpl& operator=(PrivateImpl&&) = delete;

    explicit PrivateImpl(std::string_view value)
        : m_value{value}
    {}
private:
    std::string m_value;
    int64_t m_priority = 0;
    TestQPointer *m_qImpl = nullptr;

    friend class TestQPointer;
};

class TestQPointer {
public:
    TestQPointer()
        : m_pImpl{new PrivateImpl()}
    {
        ++default_constructed;
    }

    TestQPointer(std::string_view value)
        : m_pImpl{new PrivateImpl{value}}
    {
        m_pImpl->m_qImpl = this;
        ++explicit_constructed;
    }

    TestQPointer(TestQPointer const &other)
        : m_pImpl{other.m_pImpl ? new PrivateImpl(*other.m_pImpl) : nullptr}
    {
        if (m_pImpl)
            m_pImpl->m_qImpl = this;
        ++copy_constructed;
    }

    TestQPointer(TestQPointer &&other) noexcept
    {
        swap(other);
        ++move_constructed;
    }

    TestQPointer& operator=(TestQPointer const &other)
    {
        if (m_pImpl != other.m_pImpl)
            TestQPointer{other}.swap(*this);
        ++copy_assigned;
        --copy_constructed;
        return *this;
    }

    TestQPointer& operator=(TestQPointer &&other) noexcept
    {
        if (m_pImpl != other.m_pImpl)
            TestQPointer{std::move(other)}.swap(*this);
        ++move_assigned;
        --move_constructed;
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
        if (m_pImpl)
            delete m_pImpl;
    }

    bool check() const
    {
        return m_pImpl && (this == m_pImpl->m_qImpl);
    }

    std::string const& value() const
    {
        if (empty())
            throw std::runtime_error{"Empty!"};
        return m_pImpl->m_value;
    }

    std::string& value()
    {
        if (empty())
            throw std::runtime_error{"Empty!"};
        return m_pImpl->m_value;
    }

    int64_t get_priority() const
    {
        if (empty())
            throw std::runtime_error{"Empty!"};
        return m_pImpl->m_priority;
    }

    void set_priority(int64_t value)
    {
        if (empty())
            throw std::runtime_error{"Empty!"};
        if (m_pImpl->m_priority != value)
        {
            m_pImpl->m_priority = value;
            m_pImpl->m_value = std::to_string(value);
        }
    }

    bool empty() const
    {  return !m_pImpl; }

private:
    PrivateImpl *m_pImpl = nullptr;

    friend class PrivatePimpl;
};

} // namespace test

RTTI_REGISTER
{
    rtti::global_define()
        ._namespace("test")
            ._class<test::TestQPointer>("TestQPointer")
                ._constructor<std::string_view>()
                ._method("empty", &test::TestQPointer::empty)
                ._method("check", &test::TestQPointer::check)
                ._property("priority", &test::TestQPointer::get_priority, &test::TestQPointer::set_priority)
                ._method<std::string& (test::TestQPointer::*)()>("value", &test::TestQPointer::value)
                ._method<std::string const& (test::TestQPointer::*)() const>("const_value", &test::TestQPointer::value)
            ._end()
        ._end()
    ;
}

TEST_CASE("Variant")
{
    SUBCASE("1")
    {
        reset_counters();

        using namespace std::literals;
        rtti::variant v = "Hello, World!"sv;
        auto qp = v.to<test::TestQPointer>();
        REQUIRE(qp.check());
        REQUIRE(qp.value() == "Hello, World!");
        REQUIRE(qp.get_priority() == 0);
        REQUIRE((
            (explicit_constructed == 1)
            && (default_constructed == 0)
            && (copy_constructed == 0)
            && (move_constructed == 1)
            && (copy_assigned == 0)
            && (move_assigned == 0)
        ));

        SUBCASE("Copy")
        {
            reset_counters();

            rtti::variant v = qp;
            REQUIRE(v.invoke("check") == true);
            REQUIRE(v.invoke("empty") == false);

            auto *meta_class = v.metaClass();
            REQUIRE(meta_class == rtti::MetaNamespace::global()->getNamespace("test")->getClass("TestQPointer"));

            auto rov = v.invoke("const_value");
            REQUIRE(rov == "Hello, World!"s);
            auto ev = v.invoke("value");
            ev.ref<std::string>() = "Qwerty";
            REQUIRE(rov == "Qwerty"s);

            REQUIRE_FALSE(qp.empty());
            REQUIRE(qp.value() == "Hello, World!");

            REQUIRE((
                (explicit_constructed == 0)
                && (default_constructed == 0)
                && (copy_constructed == 1)
                && (move_constructed == 0)
                && (copy_assigned == 0)
                && (move_assigned == 0)
            ));
        }

        SUBCASE("Move")
        {
            reset_counters();

            rtti::variant v = std::move(qp);
            REQUIRE(v.invoke("check") == true);
            REQUIRE(v.invoke("empty") == false);

            auto rov = v.invoke("const_value");
            REQUIRE(rov == "Hello, World!"s);
            auto ev = v.invoke("value");
            ev.ref<std::string>() = "Qwerty";
            REQUIRE(rov == "Qwerty"s);

            REQUIRE(qp.empty());

            REQUIRE((
                (explicit_constructed == 0)
                && (default_constructed == 0)
                && (copy_constructed == 0)
                && (move_constructed == 1)
                && (copy_assigned == 0)
                && (move_assigned == 0)
            ));
        }

        SUBCASE("Reference")
        {
            reset_counters();

            rtti::variant v = std::ref(qp);

            REQUIRE(v.invoke("check") == true);
            REQUIRE(v.invoke("empty") == false);
        }
    }
}
