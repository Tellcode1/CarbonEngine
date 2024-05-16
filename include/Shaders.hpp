#ifndef __SHADERS_HPP__
#define __SHADERS_HPP__

#include "stdafx.hpp"

struct ShadersSingleton
{

    NANO_SINGLETON_FUNCTION ShadersSingleton* GetSingleton() {
        static ShadersSingleton global{};
        return &global;
    }
};

ShadersSingleton* Shaders = ShadersSingleton::GetSingleton();

#endif