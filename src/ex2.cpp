#include "backend/vulkan/vulkan_context.hpp"
#include "backend/vulkan/swap_chain.hpp"
#include "backend/vulkan/ui_renderer.hpp"

#include <iostream>
#include <format>
#include <vector>
#include <random>
#include <cmath>
#include <unordered_map>

struct Particle {
    float x, y;
    float vx, vy;
};

void draw_circle(xel::backend::vulkan::UIRenderer& ui, float cx, float cy, float r,
                 glm::vec4 color, int segments = 12)
{
    constexpr float PI = 3.14159265358979f;
    for (int i = 0; i < segments; ++i) {
        float a0 = 2.0f * PI * i / segments;
        float a1 = 2.0f * PI * (i + 1) / segments;
        ui.draw_triangle(
            cx, cy,
            cx + r * std::cos(a0), cy + r * std::sin(a0),
            cx + r * std::cos(a1), cy + r * std::sin(a1),
            color);
    }
}

int main()
{
    using namespace xel::backend::vulkan;
    Window window{1200, 800, "xel"};
    VulkanContext ctx{window};
    SwapChain swap_chain{ctx, window};
    UIRenderer renderer{ctx, swap_chain};

    constexpr float PI = 3.14159265358979f;

    constexpr int   N            = 300;      // 粒子数
    constexpr float CONNECT_DIST = 80.0f;  // 连接距离阈值
    constexpr float MAX_SPEED    = 120.0f;   // 速度上限（像素/秒）
    constexpr float DOT_SIZE     = 5.0f;
    constexpr float MOUSE_RADIUS = 200.0f;  // 鼠标吸引半径
    constexpr float MOUSE_FORCE  = 400.0f;  // 鼠标吸引力
    // 随机初始化
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distX(0.0f, 1200.0f);
    std::uniform_real_distribution<float> distY(0.0f, 800.0f);
    std::uniform_real_distribution<float> distV(-MAX_SPEED, MAX_SPEED);

    std::vector<Particle> particles(N);
    for (auto& p : particles) {
        p.x  = distX(rng);
        p.y  = distY(rng);
        p.vx = distV(rng);
        p.vy = distV(rng);
    }
    float last_time = static_cast<float>(glfwGetTime());
    float mouse_x = 0.0f, mouse_y = 0.0f;

    const int cell_size = static_cast<int>(CONNECT_DIST);
    std::unordered_map<int, std::vector<int>> grid;
    grid.reserve(N * 2);

    while (!window.should_close())
    {
        glfwPollEvents();
        double mx, my;
        glfwGetCursorPos(window.get_glfw_window(), &mx, &my);
        mouse_x = static_cast<float>(mx);
        mouse_y = static_cast<float>(my);

        // 时间步长
        float now = static_cast<float>(glfwGetTime());
        float dt  = now - last_time;
        last_time = now;
        if (dt > 0.05f) dt = 0.05f;   // 防止掉帧时位置跳变

        float W = static_cast<float>(swap_chain.extent().width);
        float H = static_cast<float>(swap_chain.extent().height);

        // 更新位置、阻尼、鼠标吸引、边界反弹
        const float mr2 = MOUSE_RADIUS * MOUSE_RADIUS;
        for (auto& p : particles) {
            // 鼠标吸引
            float mdx = mouse_x - p.x;
            float mdy = mouse_y - p.y;
            float md2 = mdx * mdx + mdy * mdy;
            if (md2 < mr2 && md2 > 1.0f) {
                float md = std::sqrt(md2);
                // 力随距离衰减
                float strength = (1.0f - md / MOUSE_RADIUS) * MOUSE_FORCE;
                p.vx += (mdx / md) * strength * dt;
                p.vy += (mdy / md) * strength * dt;
            }
            // 速度上限
            float sp2 = p.vx * p.vx + p.vy * p.vy;
            if (sp2 > MAX_SPEED * MAX_SPEED) {
                float sp = std::sqrt(sp2);
                p.vx = p.vx / sp * MAX_SPEED;
                p.vy = p.vy / sp * MAX_SPEED;
            }
            // 积分
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            // 边界反弹
            if (p.x < 0) { p.x = 0; p.vx = -p.vx; }
            if (p.x > W) { p.x = W; p.vx = -p.vx; }
            if (p.y < 0) { p.y = 0; p.vy = -p.vy; }
            if (p.y > H) { p.y = H; p.vy = -p.vy; }
        }

        // 构建空间网格
        grid.clear();
        for (int i = 0; i < N; ++i) {
            int cx = static_cast<int>(particles[i].x) / cell_size;
            int cy = static_cast<int>(particles[i].y) / cell_size;
            grid[cy * 10000 + cx].push_back(i);
        }

        renderer.begin();

        // 网格加速配对，画连接线
        const float max_d2 = CONNECT_DIST * CONNECT_DIST;
        for (int i = 0; i < N; ++i) {
            const auto& p = particles[i];
            int cx = static_cast<int>(p.x) / cell_size;
            int cy = static_cast<int>(p.y) / cell_size;
            // 遍历 3×3 邻域
            for (int ox = -1; ox <= 1; ++ox) {
                for (int oy = -1; oy <= 1; ++oy) {
                    auto it = grid.find((cy + oy) * 10000 + (cx + ox));
                    if (it == grid.end()) continue;
                    for (int j : it->second) {
                        if (j <= i) continue;   // 避免重复
                        const auto& q = particles[j];
                        float dx = p.x - q.x;
                        float dy = p.y - q.y;
                        float d2 = dx * dx + dy * dy;
                        if (d2 > max_d2) continue;
                        float d = std::sqrt(d2);
                        float t = 1.0f - d / CONNECT_DIST;   // 0~1
                        // 颜色渐变：近粉紫 → 远青
                        glm::vec4 near_c{1.0f, 0.3f, 0.8f, 0.0f};
                        glm::vec4 far_c {0.3f, 0.85f, 1.0f, 0.0f};
                        glm::vec4 c = glm::mix(far_c, near_c, t);
                        // 亮度平方衰减
                        c.a = t * t * 0.8f;
                        renderer.draw_line(p.x, p.y, q.x, q.y, 1.0f, c);
                    }
                }
            }
        }

        // 画粒子本身
        for (const auto& p : particles) {
            renderer.draw_rect(
                p.x - DOT_SIZE * 0.5f,
                p.y - DOT_SIZE * 0.5f,
                DOT_SIZE, DOT_SIZE,
                {1.0f, 1.0f, 1.0f, 0.9f});
        }

        // 鼠标光标
        draw_circle(renderer, mouse_x, mouse_y, 50.0f, {1.0f, 0.5f, 0.2f, 0.05f}, 12);

        // 显示帧率
        float fps = 1.0f / (dt + 1e-5f);
        renderer.draw_text(20.0f, 20.0f, 32.0f, std::format("FPS: {:.0f}", fps), {0.0f, 1.0f, 0.0f, 0.8f});
        renderer.draw_text(20.0f, 52.0f, 32.0f, std::format("vertices: {}", renderer.vertices_count()), {0.0f, 1.0f, 0.0f, 0.8f});
        renderer.draw_text(20.0f, 84.0f, 32.0f, std::format("indices: {}", renderer.indices_count()), {0.0f, 1.0f, 0.0f, 0.8f});
        renderer.end();
    }
    ctx.device().waitIdle();

    std::cout << __FILE__ << std::endl;
}