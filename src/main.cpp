#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"
#include "Renderer.hpp"
#include "AssetLoader.hpp"
#include "DynamicDescriptors.hpp"
#include "Camera.hpp"
// #include "../Atlas/build/atlas.hpp"
#include "TextRenderer.hpp"

constexpr SDL_KeyCode EXIT_KEY = SDLK_q;

// vec4 compress(mat4& mat) {
//     return vec4(
//         mat[0][0] + mat[0][1] + mat[0][2] + mat[0][3],
//         mat[1][0] + mat[1][1] + mat[1][2] + mat[1][3],
//         mat[2][0] + mat[2][1] + mat[2][2] + mat[2][3],
//         mat[3][0] + mat[3][1] + mat[3][2] + mat[3][3]
//     );
// }

int main(void) {
    // pro::__resultFunc = rescheck;
    const auto start = std::chrono::high_resolution_clock::now();

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    TIME_FUNCTION(VulkanContext ctx = CreateVulkanContext("epic"));
    TIME_FUNCTION(Renderer rd{&ctx});

    constexpr float updateTime = 3.0f;
    float totalTime = 0.0f;
    u16 numFrames = 0;

    std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point lastFrameTime = currentTime;
    float deltaTime = 0.0;

    const char* text = "epic";

    glm::vec2 normalizedMousePosition(0.0f);
    glm::vec2 rawMousePosition(0.0f);

    const auto& initTime = std::chrono::high_resolution_clock::now() - start;
    printf("Initialized in %ld ms || %.3f s\n", std::chrono::duration_cast<std::chrono::milliseconds>(initTime).count(), std::chrono::duration_cast<std::chrono::milliseconds>(initTime).count() / 1000.0f);

    bool running = true;
    SDL_Event event;
    while(running) {

        currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<f64>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // Profiling code
        totalTime += deltaTime;
        numFrames++;
        if (totalTime > updateTime) {
            printf("%ldfps : %d frames / %.2fs = ~%f ms/frame\n", static_cast<u64>(floor(numFrames / totalTime)), numFrames, updateTime, totalTime / static_cast<f32>(numFrames));
            numFrames = 0;
            totalTime = 0.0;
        }
        
        SDL_GetMouseState(&rawMousePosition.x, &rawMousePosition.y);

        normalizedMousePosition = (rawMousePosition * 2.0f / glm::vec2(rd.renderArea.width, rd.renderArea.height)) - 1.0f;

        // Render loop
        if (rd.BeginRender()) {
            const VkCommandBuffer cmd = rd.GetDrawBuffer();

            // PUSH CONSTANT UPLOADING
            {
                struct {
                    glm::mat4 model = glm::mat4(1.0f);
                } pc{};
                
                const glm::mat4 projection = glm::ortho(0.0f, (float)rd.renderArea.width, 0.0f, (float)rd.renderArea.height, 0.0f, 1.0f);
                const glm::mat4 view = glm::lookAtRH(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                const glm::mat4 translation = glm::translate(pc.model, glm::vec3(rawMousePosition, 0.0f));
                glm::mat4 model = 
                    glm::rotate(
                        translation,
                        // static_cast<float>((glm::radians(SDL_GetTicks() / 1000.0f) * 360.0f)),
                        0.0f,
                        glm::vec3(0.0f, 0.0f, 1.0f)
                    );

                pc.model = projection * model * view;
                vkCmdPushConstants(cmd, rd.pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
            }
            
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, rd.pipeline.layout, 0, 1, &rd.descSet[rd.currentFrame], 0, nullptr);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, rd.pipeline);
            rd.turd.Render(text, cmd, rd.pipeline.layout, normalizedMousePosition);
            
            /*
            *   Draw other stuff
            */
            rd.EndRender();
        }

        while(SDL_PollEvent(&event)) {
            if ((event.type == SDL_EVENT_QUIT) || (event.type == SDL_EVENT_KEY_DOWN && event.key.keysym.sym == EXIT_KEY)) {
                running = false;
                return 0;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                rd.ResizeRenderWindow({1280, 720}, true);
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.keysym.sym)
                {
                    case SDLK_KP_ENTER:
                        text = "your mom";
                        break;

                    case SDLK_0:
                        text = "i want to die";
                        break;
                
                    default:
                        break;
                }
            }
        }
    }
    

    // Do not do cleanup because we are big bois
    return 0;
}
