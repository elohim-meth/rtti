#include "test_register.h"

template void define_std_string<std::string>(rtti::meta_define<std::string>);
template void define_std_string<std::wstring>(rtti::meta_define<std::wstring>);
