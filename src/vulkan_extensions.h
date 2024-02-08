#pragma once

#include <vulkan/vulkan.h>
#include "vk_enum_to_str.h"
#include <vector>
#include <string.h>
#include <algorithm>

const VkInstanceCreateInfo* registerInstanceExtensions( const VkInstanceCreateInfo* pCreateInfo );
const VkDeviceCreateInfo* registerDeviceExtensions (
        const VkDeviceCreateInfo* pCreateInfo,
        vk_instance_dispatch_table vtable,
        VkPhysicalDevice physicalDevice,
        PFN_vkGetPhysicalDeviceFeatures2KHR fpGetPhysicalDeviceFeatures2KHR,
        bool& driverPropertiesAvailable );


inline const VkInstanceCreateInfo* registerInstanceExtensions( const VkInstanceCreateInfo* pCreateInfo ) {

    static std::vector<const char*> enabled_extensions;
    enabled_extensions = std::vector<const char*> (pCreateInfo->ppEnabledExtensionNames,
                                                   pCreateInfo->ppEnabledExtensionNames +
                                                   pCreateInfo->enabledExtensionCount);

    enabled_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    static VkInstanceCreateInfo createInfo;
    memcpy( &createInfo, pCreateInfo, sizeof(VkInstanceCreateInfo) );
    createInfo.enabledExtensionCount = enabled_extensions.size();
    createInfo.ppEnabledExtensionNames = enabled_extensions.data();

    return &createInfo;

}


inline const VkDeviceCreateInfo* registerDeviceExtensions (
        const VkDeviceCreateInfo* pCreateInfo,
        vk_instance_dispatch_table vtable,
        VkPhysicalDevice physicalDevice,
        PFN_vkGetPhysicalDeviceFeatures2KHR fpGetPhysicalDeviceFeatures2KHR,
        bool& driverPropertiesAvailable ) {

    static std::vector<const char*> enabled_extensions;
    enabled_extensions = std::vector<const char*> (pCreateInfo->ppEnabledExtensionNames,
                                                   pCreateInfo->ppEnabledExtensionNames +
                                                   pCreateInfo->enabledExtensionCount);

    uint32_t extension_count;
    vtable.EnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vtable.EnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, available_extensions.data());

    for (auto& extension : available_extensions) {

        // maybe able to get these on API_VERSION < 1.2: VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME;
        if (extension.extensionName == std::string(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME)) {
            if (enabled_extensions.end() == std::find_if(enabled_extensions.begin(),
                            enabled_extensions.end(),
                            [&](const char*& x) { return !strcmp(x, VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME); })) {
                enabled_extensions.push_back(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME);
                driverPropertiesAvailable = true;
            }
        }

        if (extension.extensionName == std::string(VK_KHR_PRESENT_ID_EXTENSION_NAME)) {
            if (enabled_extensions.end() == std::find_if(enabled_extensions.begin(),
                            enabled_extensions.end(),
                            [&](const char*& x) { return !strcmp(x, VK_KHR_PRESENT_ID_EXTENSION_NAME); })) {
                enabled_extensions.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);
            }
        }

        if (extension.extensionName == std::string(VK_KHR_PRESENT_WAIT_EXTENSION_NAME)) {
            if (enabled_extensions.end() == std::find_if(enabled_extensions.begin(),
                            enabled_extensions.end(),
                            [&](const char*& x) { return !strcmp(x, VK_KHR_PRESENT_WAIT_EXTENSION_NAME); })) {
                enabled_extensions.push_back(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
            }
        }

    }

    static VkDeviceCreateInfo createInfo;
    memcpy( &createInfo, pCreateInfo, sizeof(VkDeviceCreateInfo) );
    createInfo.enabledExtensionCount = enabled_extensions.size();
    createInfo.ppEnabledExtensionNames = enabled_extensions.data();

    static VkPhysicalDevicePresentIdFeaturesKHR    khrPresentId;
    static VkPhysicalDevicePresentWaitFeaturesKHR  khrPresentWait;

    static VkPhysicalDeviceFeatures2 physicalDeviceFeatures2;
    physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.pNext = (void*) createInfo.pNext;

    khrPresentId.sType = (VkStructureType) VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
    khrPresentId.pNext = (void*) std::exchange(physicalDeviceFeatures2.pNext, &khrPresentId);

    khrPresentWait.sType = (VkStructureType) VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
    khrPresentWait.pNext = (void*) std::exchange(physicalDeviceFeatures2.pNext, &khrPresentWait);

    if (fpGetPhysicalDeviceFeatures2KHR)
        fpGetPhysicalDeviceFeatures2KHR(physicalDevice, &physicalDeviceFeatures2);

    SPDLOG_INFO("present_id   is supported : {}", (bool) khrPresentId.presentId );
    SPDLOG_INFO("present_wait is supported : {}", (bool) khrPresentWait.presentWait );

    createInfo.pNext = &physicalDeviceFeatures2;
    createInfo.pEnabledFeatures = nullptr; // TODO: may need to move the features?!

    return &createInfo;

}
