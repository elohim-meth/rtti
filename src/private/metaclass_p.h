#ifndef METACLASS_P_H
#define METACLASS_P_H

#include "metacontainer_p.h"

#include <rtti/metaclass.h>
#include <algorithm>

namespace rtti {
namespace internal {

    class RTTI_PRIVATE DerivedClassList
    {
    public:
        void add(MetaType_ID value)
        {
            std::unique_lock lock{m_lock};
            auto search = std::find(std::begin(m_items), std::end(m_items), value);
            if (search == std::end(m_items))
                m_items.push_back(value);
        }

        std::size_t count() const
        {
            std::shared_lock lock{m_lock};
            return m_items.size();
        }

        MetaType_ID get(std::size_t index) const
        {
            std::shared_lock lock{m_lock};
            if (index < m_items.size())
                return m_items[index];
            return MetaType_ID{};
        }

        bool find(MetaType_ID value) const
        {
            std::shared_lock lock{m_lock};
            auto search = std::find(std::begin(m_items), std::end(m_items), value);
            return (search != std::end(m_items));
        }

    private:
        mutable std::shared_mutex m_lock;
        std::vector<MetaType_ID> m_items;
    };

    class RTTI_PRIVATE BaseClassList
    {
    public:
        using item_t      = std::pair<MetaType_ID, MetaClass::cast_func_t>;
        using container_t = std::vector<item_t>;

        void add(MetaType_ID value, MetaClass::cast_func_t func)
        {
            std::unique_lock lock{m_lock};
            if (!find_imp(value))
                m_items.emplace_back(value, func);
        }

        std::size_t count() const
        {
            std::shared_lock lock{m_lock};
            return m_items.size();
        }

        MetaType_ID get(std::size_t index) const
        {
            std::shared_lock lock{m_lock};
            if (index < m_items.size())
                return m_items[index].first;
            return MetaType_ID{};
        }

        bool find(MetaType_ID value) const
        {
            std::shared_lock lock{m_lock};
            return find_imp(value);
        }

        template<typename F>
        void for_each(F &&func) const;

    private:
        bool find_imp(MetaType_ID value) const
        {
            auto search = std::find_if(std::begin(m_items), std::end(m_items),
                                       [value](auto const &item) { return (value == item.first); });
            return (search != std::end(m_items));
        }

        mutable std::shared_mutex m_lock;
        container_t m_items;
    };

    template<typename F>
    inline void BaseClassList::for_each(F &&func) const
    {
        std::shared_lock lock{m_lock};
        for (auto const &item: m_items)
        {
            if (!func(item))
                break;
        }
    }
} // namespace internal

class RTTI_PRIVATE MetaClassPrivate: public MetaContainerPrivate
{
public:
    MetaClassPrivate(std::string_view name, MetaContainer const &owner, MetaType_ID typeId)
        : MetaContainerPrivate{name, owner}
        , m_typeId{typeId}
    {}

private:
    MetaType_ID m_typeId;
    internal::BaseClassList m_baseClasses;
    internal::DerivedClassList m_derivedClasses;

    friend class rtti::MetaClass;
};

} // namespace rtti

#endif // METACLASS_P_H
