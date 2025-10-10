#include "SharedBufferPool.h"

SharedBufferPool::SharedBufferPool(size_t initialCapacity, size_t poolSize)
    : initialCapacity_(initialCapacity) {
    for (size_t i = 0; i < poolSize; ++i) {
        bufferQueue_.emplace(BufferPair{
            std::vector<float>(initialCapacity * 4 * 5),
            std::vector<unsigned int>(initialCapacity * 6)
            });
    }
}

std::pair<std::vector<float>&, std::vector<unsigned int>&> SharedBufferPool::AcquireBuffers() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    bufferAvailable_.wait(lock, [this]() { return !bufferQueue_.empty(); });

    auto buffers = std::move(bufferQueue_.front()); //NEVERMIND :) i don't trust that this will work as intended, we are returning reference to stack variable
    bufferQueue_.pop();
    return { buffers.vertexBuffer, buffers.indexBuffer };
}

void SharedBufferPool::ReleaseBuffers(std::vector<float>& vertexBuffer, std::vector<unsigned int>& indexBuffer) {
    std::lock_guard<std::mutex> lock(queueMutex_);

    vertexBuffer.clear();
    indexBuffer.clear();
    bufferQueue_.emplace(BufferPair{ std::move(vertexBuffer), std::move(indexBuffer) });

    bufferAvailable_.notify_one();
}