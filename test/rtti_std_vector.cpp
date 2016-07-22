#include "test_register.h"

template void define_std_vector<int>(rtti::meta_define<std::vector<int>>);
template void define_std_vector<std::string>(rtti::meta_define<std::vector<std::string>>);
template void define_std_vector<char const*>(rtti::meta_define<std::vector<char const*>>);
