#ifndef __CARBON_HPP__
#define __CARBON_HPP__

#include "stdafx.hpp"
#include "Renderer.hpp"

namespace carbon
{
    extern u16vec2 WindowSize;
    extern vec2 MousePosition;
    extern vec2 NormalizedMousePosition; // NDC

    // Should be called at the end of each frame.
    static void Update();
}

#endif