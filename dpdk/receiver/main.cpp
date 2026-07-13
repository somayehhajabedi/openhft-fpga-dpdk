#include "receiver.hpp"

int main(int argc, char** argv)
{
    Receiver receiver;

    if (!receiver.initialize(argc, argv))
        return -1;

    receiver.run();

    return 0;
}
