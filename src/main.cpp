#define STB_IMAGE_IMPLEMENTATION

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"
#include "Renderer.hpp"
#include "CFont.hpp"

void GetMousePositionNDC(SDL_Window* window, float* ndcX, float* ndcY) {
    f32 mouseX, mouseY;
    SDL_GetGlobalMouseState(&mouseX, &mouseY);

    // Convert to NDC
    *ndcX = 2.0f * (mouseX / static_cast<float>(Graphics->RenderExtent.width)) - 1.0f;
    *ndcY = 1.0f - 2.0f * (mouseY / static_cast<float>(Graphics->RenderExtent.height));
}

int main(void) {
    system("clear");

    const auto start = std::chrono::high_resolution_clock::now();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    const auto& RD = Renderer;

    TIME_FUNCTION(Context->Initialize("epic", 800, 600));
    TIME_FUNCTION(Renderer->Initialize());

    constexpr float updateTime = 3.0f;
    float totalTime = 0.0f;
    u16 numFrames = 0;

    using time_point = std::chrono::high_resolution_clock::time_point;

    time_point currentTime = std::chrono::high_resolution_clock::now();
    time_point lastFrameTime = currentTime;
    float dt = 0.0;
    std::string str = "gamer gamer gamer gamer gamer";

    glm::vec2 mousePosition(0.0f);

    const auto& initTime = std::chrono::high_resolution_clock::now() - start;
    printf("Initialized in %ld ms || %.3f s\n", std::chrono::duration_cast<std::chrono::milliseconds>(initTime).count(), std::chrono::duration_cast<std::chrono::milliseconds>(initTime).count() / 1000.0f);
    float zoom = 0.05f;

    cf::CFont amongus;
    cf::CFontLoadInfo infoo{};
    infoo.fontPath = "../Assets/roboto.ttf";
    cf::CFLoad(&infoo, &amongus);

    SDL_Event event;

    while(RD->running) {
        currentTime = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration<f64>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // Profiling code
        totalTime += dt;
        numFrames++;
        if (totalTime > updateTime) {
            printf("%ldfps : %d frames / %.2fs = ~%fs/frame\n", static_cast<u64>(floor(numFrames / totalTime)), numFrames, updateTime, totalTime / static_cast<f32>(numFrames));
            numFrames = 0;
            totalTime = 0.0;
        }

        vec2 rawMousePosition;
        SDL_GetMouseState(&rawMousePosition.x, &rawMousePosition.y);
        // GetMousePositionNDC(window, &mousePosition.x, &mousePosition.y);

        // std::cout << rawMousePosition.x << ", " << rawMousePosition.y << '\n';
        // std::cout << mousePosition.x << ", " << mousePosition.y << '\n';
        // std::cout << '\n';

        if (RD->BeginRender()) {
            const VkCommandBuffer cmd = RD->GetDrawBuffer();

            const f32 halfWidth = (f32)Graphics->RenderExtent.width / 2.0f;
            const f32 halfHeight = (f32)Graphics->RenderExtent.height / 2.0f;
            const glm::mat4 projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.0f, 1.0f);
            // const f32 aspectRatio = (f32)Graphics->RenderExtent.width / Graphics->RenderExtent.height;
            // const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.0f, 1.0f);
            // std::cout << "Window size : " << halfWidth << 'x' << halfHeight << '\n';

            /*
            *   Draw other stuff
            */
            RD->EndRender();
        }

        SDL_PumpEvents();

        while(SDL_PollEvent(&event)) {
            RD->ProcessEvent(&event);
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                zoom += event.wheel.y * 0.05f;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.keysym.sym)
                {
                // case SDLK_w:
                //     camPos.y -= 0.01f;
                //     break;
                // case SDLK_s:
                //     camPos.y += 0.01f;
                //     break;
                // case SDLK_a:
                //     camPos.x -= 0.01f;
                //     break;
                // case SDLK_d:
                // case SDLK_KP_ENTER:
                //     str += '\n';
                //     break;
                case SDLK_BACKSPACE:
                    if(str.length() > 0) str.pop_back();
                    break;
                case SDLK_DELETE:
                    str.clear();
                    break;
                case SDLK_TAB:
                    str += '\t';
                    break;
                case SDLK_RETURN:
                    str += '\n';
                    break;
                case SDLK_END:
                    str += '\0';
                    break;
                case SDLK_GRAVE:
                    str += 
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n"
                    "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n";
                    std::cout << str.size() << '\n';
                default:
                    // str += event.key.keysym.sym;
                    break;
                }
            }
            else if (event.type == SDL_EVENT_TEXT_INPUT) {
                str += event.text.text;
            }
        }

        // Input::ProcessEvents();
        
    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
