#include "engine.hpp"

#include <iostream>

Engine::Engine() { std::cout << "HELLO \n"; }
Engine::~Engine() { std::cout << "BYE \n"; }

void Engine::test() { std::cout << "testing \n"; }