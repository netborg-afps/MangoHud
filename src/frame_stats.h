#pragma once
#include <atomic>
#include <vulkan/vulkan.h>
#include "vulkan_update.h"
#include "atomic_signal.h"


typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point_t;
class FrameStatsStorage;


struct FrameStats {

    uint32_t frametime;
    uint32_t latency;

    time_point_t start;
    time_point_t end;

private:
friend class FrameStatsStorage;

    // there is no guarantee that these parameters won't change from
    // present to present, so we store them for each frame
    VkDevice device;
    VkSwapchainKHR swapchain;
    PFN_vkWaitForPresentKHR fpWaitForPresentKHR;

};


class FrameStatsReader {

public:

    FrameStatsReader( const FrameStatsStorage* storage );
    bool getNext( FrameStats& result );

private:

    const FrameStatsStorage* m_storage;
    uint64_t m_index;

};


class FrameStatsStorage {

friend class FrameStatsReader;

public:
    FrameStatsStorage() {

        m_producerParamsIndex.store(0);
        m_producerIndex.store(0);
        m_stats = new FrameStats[m_numStats]();

    }

    ~FrameStatsStorage() {

        delete[] m_stats;

    }

    FrameStatsReader getReader() {

        return FrameStatsReader(this);

    }

    void registerFrameStart( uint64_t presentId ) {

        time_point_t t = std::chrono::high_resolution_clock::now();

        assert( presentId > m_producerIndex );

        uint16_t index = presentId % m_numStats;
        FrameStats* stats = &m_stats[ index ];
        stats->start = t;

    }


    void registerFrameEnd( uint64_t presentId ) {

        time_point_t t = std::chrono::high_resolution_clock::now();

        // we assume these to follow in succession
        assert( presentId % m_numStats == (m_producerIndex + 1) % m_numStats );
        uint16_t index = presentId % m_numStats;

        FrameStats* stats = &m_stats[ index ];

        assert( stats->start < t );
        stats->latency = std::chrono::duration_cast<std::chrono::microseconds>(t - stats->start).count();
        stats->end = t;

        // SPDLOG_INFO("latency : {}.{} ms", stats->latency/1000, stats->latency%1000);

        m_producerIndex.store( index );

    }

    void registerWaitForPresentParams( uint64_t presentId, VkDevice& device, VkSwapchainKHR swapchain, PFN_vkWaitForPresentKHR fpWaitForPresentKHR ) {

        m_stats[ presentId % m_numStats ].device = device;
        m_stats[ presentId % m_numStats ].swapchain = swapchain;
        m_stats[ presentId % m_numStats ].fpWaitForPresentKHR = fpWaitForPresentKHR;

        m_producerParamsIndex.store( presentId );
        m_syncPresentParams.signal_one();

    }

    bool getWaitForPresentParams( uint64_t presentId, VkDevice& device, VkSwapchainKHR& swapchain, PFN_vkWaitForPresentKHR& fpWaitForPresentKHR) {

        if ((int64_t)presentId > m_producerParamsIndex.load())
            m_syncPresentParams.wait();

        if ((int64_t)presentId > m_producerParamsIndex.load())
            return false;

        device = m_stats[ presentId % m_numStats ].device;
        swapchain = m_stats[ presentId % m_numStats ].swapchain;
        fpWaitForPresentKHR = m_stats[ presentId % m_numStats ].fpWaitForPresentKHR;
        return true;

    }

    void signalWaitForPresentParams() {
        m_syncPresentParams.signal_one();
    }


private:

    // select the ringbuffer large enough so we never come into a
    // situation where the reader cannot keep up with the producer
    FrameStats* m_stats;
    static constexpr uint16_t m_numStats = 2048;
    std::atomic<uint64_t> m_producerIndex;
    std::atomic<int64_t> m_producerParamsIndex;

    AtomicSignal m_syncPresentParams = { "m_syncPresentParams", false };

};



inline FrameStatsReader::FrameStatsReader( const FrameStatsStorage* storage )
: m_storage(storage) {

    m_index = m_storage->m_producerIndex.load();

}

inline bool FrameStatsReader::getNext( FrameStats& result ) {

    uint64_t producerIndex = m_storage->m_producerIndex.load();

    // // checking for buffer underrun TODO
    // assert( m_index + (m_storage->m_numStats-5) < producerIndex );

    if (m_index == producerIndex)
        return false;

    result = m_storage->m_stats[m_index];
    m_index++;
    m_index %= m_storage->m_numStats;
    return true;

}



extern std::unique_ptr<FrameStatsStorage> g_frameStatsStorage;
