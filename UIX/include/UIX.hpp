#ifndef __UIX_HPP__
#define __UIX_HPP__

#include "stdafx.hpp"

namespace uix
{
    struct uix_context {
        int * ptr = nullptr;
        VkBuffer coreBuffer; // One buffer for the index buffer, vertex buffer
        const VkDevice device;
        const VkPhysicalDevice physDevice;
        const VkQueue queue;

        uix_context(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue) : device(device), physDevice(physDevice), queue(queue) {}
    };

    void test2(int gamer) {
        std::cout << "Hello from inside the shared library + whatever " << gamer << std::endl;
    }

    uix_context CreateContext(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue);
}

#endif