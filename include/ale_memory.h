// Aliases for smart pointers 

#pragma once
// ext
#include <memory>

#ifndef ALE_MEMORY
#define ALE_MEMORY

namespace ale {

template<typename T>
using sp = std::shared_ptr<T>;

template<typename T>
sp<T> to_sp(T x) { return std::make_shared<T>(x);};

template<typename T>
using up = std::unique_ptr<T>;

template<typename T>
up<T> to_up(T x) { return std::make_unique<T>(x);};

template<typename T>
using wp = std::weak_ptr<T>;

} // namespace ale

#endif // ALE_MEMORY
