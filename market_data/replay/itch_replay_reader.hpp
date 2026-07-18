#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

class ItchReplayReader
{
public:
    explicit ItchReplayReader(const std::string& filePath);

    bool isOpen() const;

    bool readNext(std::vector<std::uint8_t>& message);

private:
    std::ifstream stream_;
};
