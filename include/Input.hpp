// #ifndef __INPUT_HPP__
// #define __INPUT_HPP__

// #include "stdafx.hpp"

// namespace Input
// {
//     /*
//     *   The interval at which the input state is updated in milliseconds.
//     */
//     constexpr u32 INPUT_UPDATE_INTERVAL = 16;
//     u8* __PreviousFrameKeyState;
//     u8* __CurrentFrameKeyState;


//     // inline u32 __ProcessEvents(u32 i, void* __)
//     // {
//     //     memcpy(__PreviousFrameKeyState, __CurrentFrameKeyState, 512);
//     //     memcpy(__CurrentFrameKeyState, SDL_GetKeyboardState(NULL), 512);
//     //     std::cout << "Input update" << '\n';
//     //     return INPUT_UPDATE_INTERVAL;
//     // }

//     inline void ProcessEvents()
//     {
//         memcpy(__PreviousFrameKeyState, __CurrentFrameKeyState, 512);
//         memcpy(__CurrentFrameKeyState, SDL_GetKeyboardState(NULL), 512);
//     }

//     /*
//     *   Should be called AFTER the while loop that you process the events in
//     */
//     inline void Init()
//     {
//         __CurrentFrameKeyState = reinterpret_cast<u8*>(calloc(1024, 1));
//         __PreviousFrameKeyState = __CurrentFrameKeyState + 512;
//         // SDL_AddTimer(INPUT_UPDATE_INTERVAL, __ProcessEvents, nullptr);
//     }

//     /* Is currently being pressed but wasn't being pressed before */
//     inline bool IsKeyDown(u32 key)
//     {
//         return __CurrentFrameKeyState[key] && !__PreviousFrameKeyState[key];
//     }

//     /* Was pressed down but now isn't */
//     inline bool IsKeyUp(u32 key)
//     {
//         return !__CurrentFrameKeyState[key] && __PreviousFrameKeyState[key];
//     }

//     inline bool IsKeyPressed(u32 key)
//     {
//         return __CurrentFrameKeyState[key];
//     }

//     inline bool IsKeyReleased(u32 key)
//     {
//         return !__CurrentFrameKeyState[key];
//     }
// }

// #endif