#include <rtti/metadefine.h>

namespace rtti {

meta_global global_define()
{
    auto global = const_cast<MetaNamespace*>(MetaNamespace::global());
    return meta_global{global, global};
}

} // namespace rtti
