#pragma once

#include "drawable.hpp"

namespace xel::graphics
{
    class Line : public Drawable
    {
    public:
        Line(float x1, float y1, float x2, float y2, float width, glm::vec4 color)
        : x1_{x1}, y1_{y1}, x2_{x2}, y2_{y2}, width_{width}, color_{color}
        {}

        void append_to(DrawBatch& batch) const override
        {
            // 方向
            float dx = x2_ - x1_;
            float dy = y2_ - y1_;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 1e-6f) return;  // 退化线段，跳过

            // 单位方向 + 垂直方向（乘以半宽）
            float ux = dx / len;
            float uy = dy / len;
            float half_w = width_ * 0.5f;
            float px = -uy * half_w;
            float py =  ux * half_w;

            // 四个角点
            glm::vec2 a{x1_ + px, y1_ + py};
            glm::vec2 b{x1_ - px, y1_ - py};
            glm::vec2 c{x2_ - px, y2_ - py};
            glm::vec2 d{x2_ + px, y2_ + py};

            uint32_t base = static_cast<uint32_t>(batch.vertices.size());

            // 4 个顶点，UV 随便填（用白纹理采样恒为 1）
            batch.vertices.push_back({{a.x, a.y}, color_, {0.0f, 0.0f}});
            batch.vertices.push_back({{b.x, b.y}, color_, {0.0f, 0.0f}});
            batch.vertices.push_back({{c.x, c.y}, color_, {0.0f, 0.0f}});
            batch.vertices.push_back({{d.x, d.y}, color_, {0.0f, 0.0f}});

            // 两个三角形
            batch.indices.insert(batch.indices.end(), {
                base, static_cast<uint32_t>(base + 1), static_cast<uint32_t>(base + 2),
                base, static_cast<uint32_t>(base + 2), static_cast<uint32_t>(base + 3)
            });
        }
    private:
        float x1_, y1_, x2_, y2_, width_;
        glm::vec4 color_;
    };
} // namespace xel::graphics
