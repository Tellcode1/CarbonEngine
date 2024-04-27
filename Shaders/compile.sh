if [ $# -ne 1 ]; then
    echo "Usage: ./compile.sh <path>"
    echo "Please provide the CMAKE_BINARY_DIR as the only argument to this script."
    exit 1
fi

mkdir -p $1/Shaders
glslangValidator -V ./vert.glsl -S vert -o $1/Shaders/vert.spv
glslangValidator -V ./frag.glsl -S frag -o $1/Shaders/frag.spv

glslangValidator -V ./vert.text.glsl -S vert -x -o $1/Shaders/vert.text.u32
glslangValidator -V ./frag.text.glsl -S frag -x -o $1/Shaders/frag.text.u32
glslangValidator -V ./comp.glsl -S comp -o $1/Shaders/comp.text.spv

# spirv