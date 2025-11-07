#pragma once

#include <cstdint>

#include "print.hpp"

struct Data {
  uint32_t number;
  Data() : number(1) { Util::println("Default constructor: {}", number); }

  explicit Data(uint32_t n) : number(n) {
    Util::println("Parameterized constructor: {}", number);
  }

  Data(const Data& other) : number(other.number) {
    Util::println("Copy constructor: {}", number);
  }

  Data(Data&& other) noexcept : number(std::move(other.number)) {
    Util::println("Move constructor: {}", number);
    other.number = 0;
  }

  Data& operator=(const Data& other) {
    if (this != &other) {
      number = other.number;
      Util::println("Copy assignment: {}", number);
    }
    return *this;
  }

  Data& operator=(Data&& other) noexcept {
    if (this != &other) {
      number = std::move(other.number);
      Util::println("Move assignment: {}", number);
      other.number = 0;
    }
    return *this;
  }

  ~Data() { Util::println("Destructor: {}", number); }
};
