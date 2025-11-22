#pragma once
#include <type_traits>
template <typename T>
concept EventType = std::is_class_v<T>;
