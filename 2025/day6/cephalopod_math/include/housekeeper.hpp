#pragma once

#include <functional>

class Housekeeper {
private:
    std::function<void()> cleanup;

public:
    Housekeeper(std::function<void()> cleanup) : cleanup(cleanup) {}
    ~Housekeeper() { this->cleanup(); }
};

#define DEFER(label, cleanup) Housekeeper label([&]() { cleanup; })
