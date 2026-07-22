#pragma once

#include <cstdint>

class Sequencer
{
public:
    uint64_t next();

private:
    uint64_t sequence_ = 1000;
};