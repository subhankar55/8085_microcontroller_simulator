#include "IoBuffer.h"

#include <stdexcept>

namespace simulator {

void IoBuffer::EnqueueInput(uint8_t port, uint8_t value) {
    {
        std::lock_guard lock(mutex_);
        inputQueues_[port].push_back(value);
    }
    inputAvailable_.notify_all();
}

uint8_t IoBuffer::ReadInput(uint8_t port) {
    std::unique_lock lock(mutex_);
    auto& queue = inputQueues_[port];
    const auto generation = cancellationGeneration_;
    inputAvailable_.wait(lock, [&] {
        return !queue.empty() || cancellationGeneration_ != generation;
    });
    if (cancellationGeneration_ != generation) {
        throw std::runtime_error("I/O input wait cancelled");
    }
    const auto value = queue.front();
    queue.pop_front();
    return value;
}

void IoBuffer::CancelPendingReads() {
    {
        std::lock_guard lock(mutex_);
        ++cancellationGeneration_;
    }
    inputAvailable_.notify_all();
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
    {
        std::lock_guard lock(mutex_);
        for (auto& queue : inputQueues_) queue.clear();
        outputs_.clear();
        ++cancellationGeneration_;
    }
    inputAvailable_.notify_all();
}

} // namespace simulator
