#!/bin/sh

if [ $# -ne 1 ]; then
    echo "Usage: ./compile.sh <path>"
    echo "Please provide the CMAKE_BINARY_DIR as the only argument to this script."
    exit 1
fi

compile_shader(){
    echo "-- Compiling $2"
    glslangValidator -V $2 -S $3 -o $1.tmp
    spirv-opt $1.tmp -o $1
    rm $1.tmp
}

mkdir -p $1/Shaders
mkdir -p $1/Shaders/ctext
mkdir -p $1/Shaders/Generic

echo Shader Compilation start
    compile_shader $1/Shaders/vert.spv ./vert.glsl vert
    compile_shader $1/Shaders/frag.spv ./frag.glsl frag

    compile_shader $1/Shaders/shadow.vert.spv ./shadow.glsl.vert vert
    compile_shader $1/Shaders/shadow.frag.spv ./shadow.glsl.frag frag

    compile_shader $1/Shaders/Generic/Unlit.vert.spv ./Generic/Unlit.vert vert
    compile_shader $1/Shaders/Generic/Unlit.frag.spv ./Generic/Unlit.frag frag

    compile_shader $1/Shaders/vert.text.spv ./vert.text.glsl vert
    compile_shader $1/Shaders/comp.text.spv ./text.comp      comp

    compile_shader $1/Shaders/ctext/sdf.frag.spv ./ctext/sdf.frag.glsl frag
    compile_shader $1/Shaders/ctext/msdf.frag.spv ./ctext/msdf.frag.glsl frag
    compile_shader $1/Shaders/ctext/mtsdf.frag.spv ./ctext/mtsdf.frag.glsl frag
    compile_shader $1/Shaders/ctext/test.comp.spv ./ctext/test.comp.glsl comp
echo Done!