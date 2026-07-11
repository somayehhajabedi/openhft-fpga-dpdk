#include "sequencer.hpp"

uint64_t Sequencer::next()
{
    return ++sequence_;
}