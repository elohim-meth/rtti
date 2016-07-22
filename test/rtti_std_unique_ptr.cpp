#include "test_register.h"

template void define_std_unique_ptr<int>(rtti::meta_define<std::unique_ptr<int>>);
template void define_std_unique_ptr<int const>(rtti::meta_define<std::unique_ptr<int const>>);
