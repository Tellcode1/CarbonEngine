#include "../../../include/engine/object/cgameobject.hpp"
#include "../../../include/vkcb/epichelperlib.hpp"

csquare_t *ccreate_square()
{
    csquare_t *new_square = (csquare_t *)malloc(sizeof(csquare_t));
    // memset(new_square, 0, sizeof(csquare_t));
    *new_square = {};

    help::Buffers::CreateBuffer(
        sizeof(csquare_t::vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &new_square->vertex_buffer, &new_square->vertex_buffer_memory
    );

    void *mapped;
    vkMapMemory(device, new_square->vertex_buffer_memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    memcpy(mapped, csquare_t::vertices, sizeof(csquare_t::vertices));
    vkUnmapMemory(device, new_square->vertex_buffer_memory);

    return new_square;
}

void crender_square(csquare_t *shape, VkCommandBuffer buf) {
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(buf, 0, 1, &shape->vertex_buffer, offsets);
    vkCmdDraw(buf, array_len(shape->vertices) / 2, 1, 0, 0);
}

cpipeline_t *ccreate_pipeline() {
    // FIXME: Implement
    return nullptr;
}