#include "UIX.hpp"

int main() {
    std::cout << "Hello world" << std::endl;
    uix::test2(3);
    uix::uix_context ctx = uix::CreateContext(reinterpret_cast<VkDevice_T*>(0x69), nullptr, nullptr);
    std::cout << ctx.device << ctx.physDevice << ctx.queue << std::endl;
    return 0;
}
