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
uint16_t destroyed = 0;

void reset_counters()
{
    explicit_constructed = 0;
    default_constructed = 0;
    copy_constructed = 0;
    move_constructed = 0;
    copy_assigned = 0;
    move_assigned = 0;
    destroyed = 0;
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
        ++destroyed;
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
        allocate_pimpl();
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
        allocate_pimpl();
        if (m_pImpl->m_priority != value)
        {
            m_pImpl->m_priority = value;
            m_pImpl->m_value = std::to_string(value);
        }
    }

    bool empty() const
    {  return !m_pImpl; }

private:
    void allocate_pimpl()
    {
        if (!m_pImpl)
            m_pImpl = new PrivateImpl();
    }

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

TEST_CASE("Variant D-Q Pointers")
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
                && (destroyed = 1)
    ));

    SUBCASE("Metaclass")
    {
        rtti::variant v = qp;
        auto *meta_class = v.metaClass();
        REQUIRE(meta_class == rtti::MetaNamespace::global()->getNamespace("test")->getClass("TestQPointer"));
        REQUIRE(meta_class == rtti::metaType<test::TestQPointer>().metaClass());
        REQUIRE(meta_class == rtti::MetaType("test::TestQPointer").metaClass());
    }

    SUBCASE("Constructors")
    {
        auto *meta_class = rtti::metaType<test::TestQPointer>().metaClass();
        auto *def_constructor = meta_class->defaultConstructor();
        REQUIRE(def_constructor);
        auto *copy_constructor = meta_class->copyConstructor();
        REQUIRE(copy_constructor);
        auto *move_constructor = meta_class->moveConstructor();
        REQUIRE(move_constructor);

        reset_counters();
        auto v1 = def_constructor->invoke();
        v1.set_property("priority", 128);
        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 1)
                    && (copy_constructed == 0)
                    && (move_constructed == 1)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed = 1)
        ));

        reset_counters();
        auto v2 = copy_constructor->invoke(v1);
        REQUIRE(v2.get_property("priority") == INT64_C(128));
        REQUIRE(v1.invoke("const_value") == v2.invoke("const_value"));
        v2.set_property("priority", 256);
        REQUIRE(v2.invoke("const_value") == "256"s);
        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 0)
                    && (copy_constructed == 1)
                    && (move_constructed == 1)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed == 1)
        ));

        reset_counters();
        rtti::variant v3;
        REQUIRE_THROWS_AS(v3 = move_constructor->invoke(v2), rtti::bad_variant_cast);
        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 0)
                    && (copy_constructed == 0)
                    && (move_constructed == 0)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed == 0)
        ));

        REQUIRE_NOTHROW(v3 = move_constructor->invoke(std::move(v2)));
        REQUIRE(v2.invoke("check") == false);
        REQUIRE(v3.invoke("check") == true);
        REQUIRE(v3.get_property("priority") == INT64_C(256));
        REQUIRE(v3.invoke("const_value") == "256"s);
        REQUIRE(v3.invoke("const_value").eq("256"));
        REQUIRE(v3.invoke("const_value").eq(256));
        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 0)
                    && (copy_constructed == 0)
                    && (move_constructed == 4)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed == 1)
        ));

        reset_counters();
        v1 = v3;
        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 0)
                    && (copy_constructed == 1)
                    && (move_constructed == 3)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed == 1)
        ));
    }

    SUBCASE("Copy")
    {
        reset_counters();

        rtti::variant v = qp;
        REQUIRE(v.invoke("check") == true);
        REQUIRE(v.invoke("empty") == false);

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
                    && (destroyed == 0)
        ));

        auto *meta_class = v.metaClass();
        auto *meta_prop = meta_class->getProperty("priority");
        REQUIRE(meta_prop);
        meta_prop->set(v, 100);
        REQUIRE(meta_prop->get(v) == INT64_C(100));
        REQUIRE(meta_prop->get(v).eq(100));
        REQUIRE(rov == "100"s);
        REQUIRE(rov.eq("100"));
        REQUIRE(rov.eq(100));

        v.set_property("priority", 256);
        REQUIRE(v.get_property("priority").eq(256));
        REQUIRE(meta_prop->get(v).eq(256));
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
        REQUIRE_FALSE(qp.check());

        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 0)
                    && (copy_constructed == 0)
                    && (move_constructed == 1)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed == 0)
        ));
    }

    SUBCASE("Reference")
    {
        reset_counters();

        rtti::variant v = std::ref(qp);
        REQUIRE(v.invoke("check") == true);
        REQUIRE(v.invoke("empty") == false);

        auto rov = v.invoke("const_value");
        REQUIRE(rov == "Hello, World!"s);
        auto ev = v.invoke("value");
        ev.ref<std::string>() = "Qwerty";
        REQUIRE(rov == "Qwerty"s);
        REQUIRE(qp.value() == "Qwerty");

        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 0)
                    && (copy_constructed == 0)
                    && (move_constructed == 0)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed == 0)
        ));

        v.set_property("priority", 1024);
        REQUIRE(v.get_property("priority") == INT64_C(1024));
        REQUIRE(v.get_property("priority").eq(1024));
        REQUIRE(qp.get_priority() == 1024);
        REQUIRE(rov == "1024"s);

        auto *meta_class = v.metaClass();
        REQUIRE(meta_class);
        auto *move_constructor = meta_class->moveConstructor();
        REQUIRE(move_constructor);
        auto mv = move_constructor->invoke(std::move(v));

        REQUIRE_FALSE(qp.check());
        REQUIRE(v.invoke("check") == false);
        REQUIRE(mv.invoke("check") == true);

        REQUIRE_THROWS_AS(rov = v.invoke("const_value"), std::runtime_error);
        REQUIRE_NOTHROW(rov = mv.invoke("const_value"));
        REQUIRE(rov == "1024"s);
    }

    SUBCASE("Const reference")
    {
        reset_counters();

        rtti::variant v = std::cref(qp);
        REQUIRE(v.invoke("check") == true);
        REQUIRE(v.invoke("empty") == false);

        auto rov = v.invoke("const_value");
        REQUIRE(rov == "Hello, World!"s);
        REQUIRE(rov.eq("Hello, World!"));

        rtti::variant ev;
        REQUIRE_THROWS_AS(ev = v.invoke("value"), rtti::bad_variant_cast);
        REQUIRE_THROWS_AS(v.set_property("priority", 1024), rtti::bad_variant_cast);
        REQUIRE(v.get_property("priority") == INT64_C(0));

        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 0)
                    && (copy_constructed == 0)
                    && (move_constructed == 0)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed == 0)
        ));

        auto *meta_class = v.metaClass();
        REQUIRE(meta_class);
        auto *move_constructor = meta_class->moveConstructor();
        REQUIRE(move_constructor);

        // This will be implicitly copy constructed and then move constructed on copy argument!
        auto mv = move_constructor->invoke(std::move(v));
        REQUIRE(qp.check());
        REQUIRE(v.invoke("check") == true);
        REQUIRE(mv.invoke("check") == true);

        REQUIRE((
                    (explicit_constructed == 0)
                    && (default_constructed == 0)
                    && (copy_constructed == 1)
                    && (move_constructed == 5)
                    && (copy_assigned == 0)
                    && (move_assigned == 0)
                    && (destroyed == 4)
        ));

    }
}
