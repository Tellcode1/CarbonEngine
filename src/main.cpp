#define STB_IMAGE_IMPLEMENTATION

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"
#include "Renderer.hpp"
#include "TextRenderer.hpp"

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

    const auto& textRD = TextRenderer;
    const auto& RD = Renderer;

    TIME_FUNCTION(Context->Initialize("epic", 800, 600));
    TIME_FUNCTION(Renderer->Initialize());
    TIME_FUNCTION(TextRenderer->Initialize());

    constexpr float updateTime = 3.0f;
    float totalTime = 0.0f;
    u16 numFrames = 0;

    using time_point = std::chrono::high_resolution_clock::time_point;

    time_point currentTime = std::chrono::high_resolution_clock::now();
    time_point lastFrameTime = currentTime;
    float dt = 0.0;
    std::string str = "gamer gamer gamer gamer gamer";

    glm::vec2 mousePosition(0.0f);

    NanoFont defaultFont;

    TIME_FUNCTION(textRD->LoadFont("../Assets/roboto.ttf", 200, &defaultFont));

    NanoTextAlignmentVertical vertical = TEXT_VERTICAL_ALIGN_TOP;
    NanoTextAlignmentHorizontal horizontal = TEXT_HORIZONTAL_ALIGN_LEFT;

    const auto& initTime = std::chrono::high_resolution_clock::now() - start;
    printf("Initialized in %ld ms || %.3f s\n", std::chrono::duration_cast<std::chrono::milliseconds>(initTime).count(), std::chrono::duration_cast<std::chrono::milliseconds>(initTime).count() / 1000.0f);
    float zoom = 0.05f;

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

            TextRenderInfo info;
            info.scale = zoom;
            info.x = rawMousePosition.x;
            info.y = rawMousePosition.y;
            info.horizontal = horizontal;
            info.vertical = vertical;
            info.line = str;

            TextRenderInfo info2;
            info2.scale = zoom;
            info2.x = -halfWidth;
            info2.y = -halfHeight;
            info2.horizontal = TEXT_HORIZONTAL_ALIGN_LEFT;
            info2.vertical = TEXT_VERTICAL_ALIGN_TOP;
            info2.line = "(" + std::to_string(-halfWidth) + ", " + std::to_string(-halfHeight) + ")";
            
            TextRenderInfo info3;
            info3.scale = zoom;
            info3.x = halfWidth;
            info3.y = -halfHeight;
            info3.horizontal = TEXT_HORIZONTAL_ALIGN_RIGHT;
            info3.vertical = TEXT_VERTICAL_ALIGN_TOP;
            info3.line = "(" + std::to_string(halfWidth) + ", " + std::to_string(-halfHeight) + ")";

            TextRenderInfo info4;
            info4.scale = zoom;
            info4.x = -halfWidth;
            info4.y = halfHeight;
            info4.horizontal = TEXT_HORIZONTAL_ALIGN_LEFT;
            info4.vertical = TEXT_VERTICAL_ALIGN_BOTTOM;
            info4.line = "(" + std::to_string(-halfWidth) + ", " + std::to_string(halfHeight) + ")";
            
            TextRenderInfo info5;
            info5.scale = zoom;
            info5.x =  halfWidth;
            info5.y =  halfHeight;
            info5.horizontal = TEXT_HORIZONTAL_ALIGN_RIGHT;
            info5.vertical = TEXT_VERTICAL_ALIGN_BOTTOM;
            info5.line = "(" + std::to_string(halfWidth) + ", " + std::to_string(halfHeight) + ")";

            TextRenderInfo info6;
            info6.scale = (sinf(SDL_GetTicks() / 1000.0f * (2 * M_PI / 5.0f)) + 1.5f) / 5.0f;
            info6.x =  0.0f;
            info6.y =  0.0f;
            info6.horizontal = TEXT_HORIZONTAL_ALIGN_CENTER;
            info6.vertical = TEXT_VERTICAL_ALIGN_CENTER;
            info6.line = "amongus???";

            textRD->BeginRender();

            // textRD->AddToTextRenderQueue("gamering", zoom, -400.0f, -300.0f, horizontal, vertical);
            textRD->Render(info, &defaultFont);
            textRD->Render(info3, &defaultFont);
            textRD->Render(info5, &defaultFont);
            textRD->Render(info2, &defaultFont);
            textRD->Render(info6, &defaultFont);
            textRD->Render(info4, &defaultFont);

            textRD->EndRender(cmd, projection);

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
                case SDLK_F9:
                    vertical = TEXT_VERTICAL_ALIGN_TOP;
                    break;
                case SDLK_F10:
                    vertical = TEXT_VERTICAL_ALIGN_CENTER;
                    break;
                case SDLK_F11:
                    vertical = TEXT_VERTICAL_ALIGN_BOTTOM;
                    break;
                case SDLK_F12:
                    vertical = TEXT_VERTICAL_ALIGN_BASELINE;
                    break;
                case SDLK_F5:
                    horizontal = TEXT_HORIZONTAL_ALIGN_LEFT;
                    break;
                case SDLK_F6:
                    horizontal = TEXT_HORIZONTAL_ALIGN_CENTER;
                    break;
                case SDLK_F7:
                    horizontal = TEXT_HORIZONTAL_ALIGN_RIGHT;
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
