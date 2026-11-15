#pragma once

#include <array>
#include <condition_variable>
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
    void CancelPendingReads();
    void WriteOutput(uint8_t port, uint8_t value);
    std::vector<IoOutput> DrainOutputs();
    void Reset();

private:
    std::array<std::deque<uint8_t>, 256> inputQueues_;
    std::deque<IoOutput> outputs_;
    std::mutex mutex_;
    std::condition_variable inputAvailable_;
    std::uint64_t cancellationGeneration_ = 0;
};

} // namespace simulator
