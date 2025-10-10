#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

class SharedBufferPool {
public:
    SharedBufferPool(size_t initialCapacity, size_t poolSize);
    std::pair<std::vector<float>&, std::vector<unsigned int>&> AcquireBuffers();
    void ReleaseBuffers(std::vector<float>& vertexBuffer, std::vector<unsigned int>& indexBuffer);

private:
    struct BufferPair {
        std::vector<float> vertexBuffer;
        std::vector<unsigned int> indexBuffer;
    };

    std::queue<BufferPair> bufferQueue_;
    std::mutex queueMutex_;
    std::condition_variable bufferAvailable_;
    size_t initialCapacity_;
};