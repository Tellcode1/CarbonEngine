#include "cobject.hpp"

void cvk_bind_pipeline(const VkCommandBuffer cmd, const cvkpipeline *pipeline)
{
    if (pipeline->flags & CVK_PIPELINE_RENDER_DISABLE)
        LOG_ERROR("Trying to bind a pipeline for a non rendered object?");

    if (pipeline->flags & CVK_PIPELINE_DESCRIPTORS_ENABLED)
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, 0, pipeline->desc_sets.size(), pipeline->desc_sets.data(), 0, nullptr);

    if (pipeline->flags & CVK_PIPELINE_PUSH_CONSTANTS_ENABLE)
        vkCmdPushConstants(cmd, pipeline->pipeline_layout, pipeline->push_constant_shader_stages, 0, pipeline->push_constant_size, pipeline->push_constant_data);

    if (pipeline->flags &  CVK_PIPELINE_INDEXING_ENABLE)
        vkCmdBindIndexBuffer(cmd, pipeline->index_buffer, pipeline->index_buffer_offset, VK_INDEX_TYPE_UINT32);

    if (pipeline->vertex_buffers.size() != 0)
        vkCmdBindVertexBuffers(cmd, 0, pipeline->vertex_buffers.size(), pipeline->vertex_buffers.data(), pipeline->vertex_buffer_offsets.data());
}
