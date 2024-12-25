# Specify the target system (Windows 64-bit)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the C and C++ compilers
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# Specify additional flags (optional)
set(CMAKE_C_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++")
