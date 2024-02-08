// this file contains definitions of future vulkan header files (needs repo update)

#pragma once

constexpr uint32_t VK_STRUCTURE_TYPE_PRESENT_ID_KHR = 1000294000;
constexpr uint32_t VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR = 1000294001;
constexpr uint32_t VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR = 1000248000;

// VK_KHR_present_id is a preprocessor guard. Do not pass it to API calls.
#define VK_KHR_present_id 1
#define VK_KHR_PRESENT_ID_SPEC_VERSION    1
#define VK_KHR_PRESENT_ID_EXTENSION_NAME  "VK_KHR_present_id"
typedef struct VkPresentIdKHR {
    VkStructureType    sType;
    const void*        pNext;
    uint32_t           swapchainCount;
    const uint64_t*    pPresentIds;
} VkPresentIdKHR;

typedef struct VkPhysicalDevicePresentIdFeaturesKHR {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           presentId;
} VkPhysicalDevicePresentIdFeaturesKHR;


// VK_KHR_present_wait is a preprocessor guard. Do not pass it to API calls.
#define VK_KHR_present_wait 1
#define VK_KHR_PRESENT_WAIT_SPEC_VERSION  1
#define VK_KHR_PRESENT_WAIT_EXTENSION_NAME "VK_KHR_present_wait"
typedef struct VkPhysicalDevicePresentWaitFeaturesKHR {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           presentWait;
} VkPhysicalDevicePresentWaitFeaturesKHR;

typedef VkResult (VKAPI_PTR *PFN_vkWaitForPresentKHR)(VkDevice device, VkSwapchainKHR swapchain, uint64_t presentId, uint64_t timeout);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL vkWaitForPresentKHR(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    uint64_t                                    presentId,
    uint64_t                                    timeout);
#endif
