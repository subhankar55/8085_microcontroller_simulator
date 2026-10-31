#include "IoBuffer.h"

namespace simulator {

void IoBuffer::EnqueueInput(uint8_t port, uint8_t value) {
    std::lock_guard lock(mutex_);
    inputQueues_[port].push_back(value);
}

uint8_t IoBuffer::ReadInput(uint8_t port) {
    std::lock_guard lock(mutex_);
    auto& queue = inputQueues_[port];
    if (queue.empty()) return 0;
    const auto value = queue.front();
    queue.pop_front();
    return value;
}

void IoBuffer::WriteOutput(uint8_t port, uint8_t value) {
    std::lock_guard lock(mutex_);
    outputs_.push_back({ port, value });
}

std::vector<IoOutput> IoBuffer::DrainOutputs() {
    std::lock_guard lock(mutex_);
    std::vector<IoOutput> outputs(outputs_.begin(), outputs_.end());
    outputs_.clear();
    return outputs;
}

void IoBuffer::Reset() {
    std::lock_guard lock(mutex_);
    for (auto& queue : inputQueues_) queue.clear();
    outputs_.clear();
}

} // namespace simulator
