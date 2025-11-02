#pragma once
#include <format>
#include <iostream>

namespace Util {

template <class... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
  std::cout << std::format(fmt, std::forward<Args>(args)...);
}

}  // namespace Util