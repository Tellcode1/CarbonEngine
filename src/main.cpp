#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"
#include "Renderer.hpp"
#include "AssetLoader.hpp"
#include "DynamicDescriptors.hpp"
#include "Camera.hpp"
// #include "../Atlas/build/atlas.hpp"
#include "TextRenderer.hpp"

constexpr SDL_KeyCode EXIT_KEY = SDLK_ESCAPE;

// vec4 compress(mat4& mat) {
//     return vec4(
//         mat[0][0] + mat[0][1] + mat[0][2] + mat[0][3],
//         mat[1][0] + mat[1][1] + mat[1][2] + mat[1][3],
//         mat[2][0] + mat[2][1] + mat[2][2] + mat[2][3],
//         mat[3][0] + mat[3][1] + mat[3][2] + mat[3][3]
//     );
// }

inline u32 clamp(u32 n, u32 min, u32 max) {
    return (n < min) ? min : (n > max) ? max : n;
}

int main(void) {
    system("clear");
    const auto start = std::chrono::high_resolution_clock::now();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    TIME_FUNCTION(VulkanContext ctx = CreateVulkanContext("epic"));
    TIME_FUNCTION(RD->Initialize(&ctx));

    constexpr float updateTime = 3.0f;
    float totalTime = 0.0f;
    u16 numFrames = 0;

    std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point lastFrameTime = currentTime;
    float dt = 0.0;

    std::string str = "gamer gamer gamer gamer gamer";

    glm::vec2 normalizedMousePosition(0.0f);
    glm::vec2 rawMousePosition(0.0f);

    // Input::Init();

    TextRendererAlignmentVertical vertical = TEXT_VERTICAL_ALIGN_CENTER;
    TextRendererAlignmentHorizontal horizontal = TEXT_HORIZONTAL_ALIGN_LEFT;

    const auto& initTime = std::chrono::high_resolution_clock::now() - start;
    printf("Initialized in %ld ms || %.3f s\n", std::chrono::duration_cast<std::chrono::milliseconds>(initTime).count(), std::chrono::duration_cast<std::chrono::milliseconds>(initTime).count() / 1000.0f);
    float zoom = 1.0f;
    // bool running = true;
    SDL_Event event;
    // while(running) {
    while(true) {
        currentTime = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration<f64>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // Profiling code
        totalTime += dt;
        numFrames++;
        if (totalTime > updateTime) {
            printf("%ldfps : %d frames / %.2fs = ~%f ms/frame\n", static_cast<u64>(floor(numFrames / totalTime)), numFrames, updateTime, totalTime / static_cast<f32>(numFrames));
            numFrames = 0;
            totalTime = 0.0;
        }
        
        SDL_GetMouseState(&rawMousePosition.x, &rawMousePosition.y);

        normalizedMousePosition = (rawMousePosition * 2.0f / glm::vec2(RD->renderArea.width, RD->renderArea.height)) - 1.0f;

        // Render loop
        if (RD->BeginRender()) {
            const VkCommandBuffer cmd = RD->GetDrawBuffer();
            // rd.turd.imageAvailableSemaphore = rd.imageAvailableSemaphore[rd.currentFrame];
            // rd.turd.InFlightFence = rd.InFlightFence[rd.currentFrame];
            // rd.turd.renderingFinishedSemaphore = rd.renderingFinishedSemaphore[rd.currentFrame];

            // PUSH CONSTANT UPLOADING
            {
                struct {
                    glm::mat4 model = glm::mat4(1.0f);
                    vec2 texSize;
                } pc{};
                
                const glm::mat4 projection = glm::ortho(0.0f, (float)RD->renderArea.width, 0.0f, (float)RD->renderArea.height, 0.0f, 1.0f);
                // const glm::mat4 projection = glm::ortho(0.0f, 80.0f, 0.0f, 60.0f, 0.0f, 1.0f);
                const glm::mat4 view = glm::lookAtRH(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                // const glm::mat4 translation = glm::translate(pc.model, glm::vec3(rawMousePosition + glm::vec2(sinf32(SDL_GetTicks() / 100.0f) * 25.0f, cosf32(SDL_GetTicks() / 100.0f) * 25.0f), 0.0f));
                glm::mat4 model = 
                    glm::rotate(
                        // translation,
                        glm::mat4(1.0f),
                        // static_cast<float>((glm::radians(SDL_GetTicks() / 1000.0f) * 360.0f)),
                        0.0f,
                        glm::vec3(0.0f, 0.0f, 1.0f)
                    );

                pc.model = projection * model * view;
                pc.texSize = glm::vec2(textRD->bmpWidth, textRD->bmpHeight);
                vkCmdPushConstants(cmd, textRD->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
            }

            textRD->AddToTextRenderQueue(str, zoom, rawMousePosition.x, rawMousePosition.y, horizontal, vertical);
            textRD->Render(cmd);
            
            /*
            *   Draw other stuff
            */
            RD->EndRender();
        }

        while(SDL_PollEvent(&event)) {
            if ((event.type == SDL_EVENT_QUIT) || (event.type == SDL_EVENT_KEY_DOWN && event.key.keysym.sym == EXIT_KEY)) {
                return 0;
            } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                zoom += event.wheel.y * 0.25f;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.keysym.sym)
                {
                    std::size_t epic;
                    std::size_t nl;
                    case SDLK_KP_ENTER:
                        str += '\n';
                        break;
                    case SDLK_BACKSPACE:
                        if(str.length() > 0) str.pop_back();
                        break;
                    case SDLK_DELETE:
                        str.clear();
                        break;
                    case SDLK_LCTRL:
                        epic = str.find_last_of(' ');
                        nl = str.find_last_of('\n');
                        // space priority
                        if (epic != std::string::npos && epic > nl)
                            str = str.substr(0, epic - 1);
                        else if (nl != std::string::npos)
                            str = str.substr(0, nl);
                        else
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
                    case SDLK_SPACE:
                        str += ' ';
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
                    case SDLK_F5:
                        horizontal = TEXT_HORIZONTAL_ALIGN_LEFT;
                        break;
                    case SDLK_F6:
                        horizontal = TEXT_HORIZONTAL_ALIGN_CENTER;
                        break;
                    case SDLK_F7:
                        horizontal = TEXT_HORIZONTAL_ALIGN_RIGHT;
                        break;
                    case SDLK_BACKQUOTE:
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
