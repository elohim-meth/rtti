#ifndef METACLASS_P_H
#define METACLASS_P_H

#include "metaclass.h"
#include "metacontainer_p.h"

#include <algorithm>

namespace rtti {

namespace internal {

class DLL_LOCAL DerivedClassList
{
public:
    void add(MetaType_ID value) noexcept
    {
        if (!find(value))
            m_items.push_back(value);
    }

    std::size_t count() const noexcept
    {
        return m_items.size();
    }

    MetaType_ID get(std::size_t index) const noexcept
    {
        if (index < m_items.size())
            return m_items[index];
        return MetaType_ID{};
    }

    bool find(MetaType_ID value) const
    {
        auto search = std::find(std::begin(m_items), std::end(m_items), value);
        return (search != std::end(m_items));
    }

    auto begin() const noexcept -> std::vector<MetaType_ID>::const_iterator
    {
        return m_items.begin();
    }

    auto end() const noexcept -> std::vector<MetaType_ID>::const_iterator
    {
        return m_items.end();
    }
private:
    std::vector<MetaType_ID> m_items;
};

class DLL_LOCAL BaseClassList
{
public:
    using item_t = std::pair<MetaType_ID, MetaClass::cast_func_t>;
    using container_t = std::vector<item_t>;

    void add(MetaType_ID value, MetaClass::cast_func_t func) noexcept
    {
        if (!find(value))
            m_items.emplace_back(value, func);
    }

    std::size_t count() const noexcept
    {
        return m_items.size();
    }

    MetaType_ID get(std::size_t index) const noexcept
    {
        if (index < m_items.size())
            return m_items[index].first;
        return MetaType_ID{};
    }

    bool find(MetaType_ID value) const
    {
        auto search = std::find_if(std::begin(m_items), std::end(m_items),
                                   [value](const item_t &item) {
            return (value == item.first);
        });
        return (search != std::end(m_items));
    }

    auto begin() const noexcept -> container_t::const_iterator
    {
        return m_items.begin();
    }

    auto end() const noexcept -> container_t::const_iterator
    {
        return m_items.end();
    }
private:
    container_t m_items;
};

} // namespace internal

class DLL_LOCAL MetaClassPrivate: public MetaContainerPrivate
{
public:
    MetaClassPrivate(const char *name, const MetaContainer &owner, MetaType_ID typeId)
        : MetaContainerPrivate{name, owner}, m_typeId{typeId}
    {}
private:
    MetaType_ID m_typeId;
    internal::BaseClassList m_baseClasses;
    internal::DerivedClassList m_derivedClasses;

    friend class rtti::MetaClass;
};

} // namespace rtti

#endif // METACLASS_P_H

