#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

namespace simulator {

struct IoOutput {
    uint8_t port;
    uint8_t value;
};

class IoBuffer {
public:
    void EnqueueInput(uint8_t port, uint8_t value);
    uint8_t ReadInput(uint8_t port);
    void WriteOutput(uint8_t port, uint8_t value);
    std::vector<IoOutput> DrainOutputs();
    void Reset();

private:
    std::array<std::deque<uint8_t>, 256> inputQueues_;
    std::deque<IoOutput> outputs_;
    std::mutex mutex_;
};

} // namespace simulator
