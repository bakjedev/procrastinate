#pragma once

#include <format>
#include <iostream>

namespace Util
{
  // basically std::print from c++23
  template<class... Args>
  void print(std::format_string<Args...> fmt, Args&&... args)
  {
    std::cout << std::format(fmt, std::forward<Args>(args)...);
  }

  template<class... Args>
  void println(std::format_string<Args...> fmt, Args&&... args)
  {
    std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
  }
} // namespace Util
