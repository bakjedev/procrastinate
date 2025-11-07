#pragma once

#include <cstdint>

#include "print.hpp"

struct Data {
  std::unique_ptr<int> number;
  Data() : number(std::make_unique<int>(1)) {
    Util::println("Default constructor: {}", *number);
  }

  explicit Data(uint32_t n) : number(std::make_unique<int>(n)) {
    Util::println("Parameterized constructor: {}", *number);
  }

  Data(const Data& other) : number(std::make_unique<int>(*other.number)) {
    Util::println("Copy constructor: {}", *number);
  }

  Data(Data&& other) noexcept : number(std::move(other.number)) {
    Util::println("Move constructor: {}", number ? *number : -1);
  }

  Data& operator=(const Data& other) {
    if (this != &other) {
      number = std::make_unique<int>(*other.number);
      Util::println("Copy assignment: {}", *number);
    }
    return *this;
  }

  Data& operator=(Data&& other) noexcept {
    if (this != &other) {
      number = std::move(other.number);
      Util::println("Move assignment: {}", number ? *number : -1);
    }
    return *this;
  }

  ~Data() { Util::println("Destructor: {}", number ? *number : -1); }
};
