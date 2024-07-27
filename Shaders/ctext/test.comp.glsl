#version 450
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(set=0,binding=0, std430) readonly buffer BufferIn {
    uint glyph_count;
    vec4 glyphs[ ];
} buff_in;

layout(set=0,binding=1, std430) writeonly buffer BufferOut {
    vec4 positions_and_texcoords[ ];
} buff_out;

void main() {
    for (int i = 0; i < buff_in.glyph_count; i++) {
        buff_out.positions_and_texcoords[i] = buff_in.glyphs[i];
    }
}