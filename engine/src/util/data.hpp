#pragma once

#include <cstdint>
#include <memory>

#include "print.hpp"

struct Data
{
  std::unique_ptr<int> number;
  Data() : number(std::make_unique<int>(1)) { util::println("Default constructor: {}", *number); }

  explicit Data(uint32_t n) : number(std::make_unique<int>(n))
  {
    util::println("Parameterized constructor: {}", *number);
  }

  Data(const Data& other) : number(std::make_unique<int>(*other.number))
  {
    util::println("Copy constructor: {}", *number);
  }

  Data(Data&& other) noexcept : number(std::move(other.number))
  {
    util::println("Move constructor: {}", number ? *number : -1);
  }

  Data& operator=(const Data& other)
  {
    if (this != &other)
    {
      number = std::make_unique<int>(*other.number);
      util::println("Copy assignment: {}", *number);
    }
    return *this;
  }

  Data& operator=(Data&& other) noexcept
  {
    if (this != &other)
    {
      number = std::move(other.number);
      util::println("Move assignment: {}", number ? *number : -1);
    }
    return *this;
  }

  ~Data() { util::println("Destructor: {}", number ? *number : -1); }
};
