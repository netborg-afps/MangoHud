#pragma once

#include <vulkan/vulkan.h> // TODO: make .cpp so this is not in header
#include "vulkan_update.h"
#include "frame_stats.h"



/**
 *  assumes present_id and present_wait are enabled
 */
class Presenter {

public:

    struct Config {

        std::atomic<int8_t> render_ahead_limit = { 1 };

    };


    Presenter()
    : m_config() {

        assert( g_frameStatsStorage == nullptr );

        m_presentId = 1;

        m_presentWaitThreadIsRunning.store(true);
        g_frameStatsStorage = std::make_unique<FrameStatsStorage>();
        m_presentWaitThread = std::make_unique<std::thread> (
            presentWaitRun,
            &m_presentWaitThreadIsRunning);

    }

    ~Presenter() {

        m_presentWaitThreadIsRunning.store(false);
        g_frameStatsStorage->signalWaitForPresentParams();

        m_presentWaitThread->join();
        m_presentWaitThread.reset();

    }

    VkResult present( VkQueue queue, const VkPresentInfoKHR* pPresentInfo, VkDevice device, PFN_vkQueuePresentKHR& fpQueuePresentKHR, PFN_vkWaitForPresentKHR& fpWaitForPresentKHR ) {

        // handle presentId only for one swapchain per frame
        assert( pPresentInfo->swapchainCount == 1 );

        VkPresentInfoKHR presentInfo;
        memcpy( &presentInfo, pPresentInfo, sizeof(VkPresentInfoKHR) );
        VkPresentIdKHR presentIdKHR;
        addPresentIdKHR( &presentInfo, &presentIdKHR );

        VkResult result = fpQueuePresentKHR(queue, &presentInfo);

        g_frameStatsStorage->registerWaitForPresentParams(m_presentId, device,  *(presentInfo.pSwapchains), fpWaitForPresentKHR);

        int8_t render_ahead_limit = m_config.render_ahead_limit.load();

        if (render_ahead_limit >= 0 && (int64_t) m_presentId >= render_ahead_limit) {
            fpWaitForPresentKHR(
                device,
                (VkSwapchainKHR) *(presentInfo.pSwapchains),
                m_presentId - render_ahead_limit,
                std::numeric_limits<uint64_t>::max()
            );
        }

        ++m_presentId;
        g_frameStatsStorage->registerFrameStart(m_presentId);

        return result;

    }

    void setRenderAheadLimit( int8_t value )
        { m_config.render_ahead_limit.store(value); }


private:

    static void presentWaitRun( std::atomic<bool>* isRunning ) {

        uint64_t presentId = 1;
        while (isRunning->load()) {

            VkDevice device;
            VkSwapchainKHR swapchain;
            PFN_vkWaitForPresentKHR fpWaitForPresentKHR;

            if (!g_frameStatsStorage->getWaitForPresentParams(presentId, device, swapchain, fpWaitForPresentKHR))
                continue;

            // need to call it with a timeout (50 ms) to be able to shut the thread down
            VkResult res = fpWaitForPresentKHR( device, swapchain, presentId, 50'000'000 );
            if (res == VK_TIMEOUT)
                continue;

            g_frameStatsStorage->registerFrameEnd(presentId);
            presentId++;

        }
    }

    void addPresentIdKHR( VkPresentInfoKHR* presentInfo, VkPresentIdKHR* presentIdKHR ) {

        VkBaseOutStructure* pNext = (VkBaseOutStructure*) presentInfo->pNext;
        while (pNext != nullptr) {
            if (pNext->sType == VK_STRUCTURE_TYPE_PRESENT_ID_KHR) {
                VkPresentIdKHR* foundPresentIdKHR = (VkPresentIdKHR*) pNext;
                foundPresentIdKHR->swapchainCount = 1;
                foundPresentIdKHR->pPresentIds    = &m_presentId;
                return;
            }
            pNext = pNext->pNext;
        }

        presentIdKHR->sType = (VkStructureType) VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
        presentIdKHR->swapchainCount = 1;
        presentIdKHR->pPresentIds    = &m_presentId;
        presentIdKHR->pNext          = const_cast<void*>(std::exchange(presentInfo->pNext, presentIdKHR));

    }

    Config m_config;
    uint64_t m_presentId;

    std::vector< FrameStats > m_stats;

    std::unique_ptr<std::thread> m_presentWaitThread;
    std::atomic<bool> m_presentWaitThreadIsRunning;

};


extern std::unique_ptr<Presenter> g_presenter;
