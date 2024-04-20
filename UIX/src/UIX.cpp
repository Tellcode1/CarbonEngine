#include "UIX.hpp"
// #include "include/UIX.hpp"

uix::uix_context uix::CreateContext(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue)
{
    uix_context ctx(device, physDevice, queue);
    return ctx;
}