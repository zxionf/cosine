#include "graphics/drawable.hpp"
#include "ui/core/progress.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

namespace
{
using namespace xel;

bool near(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) <= eps;
}

size_t vertex_count(const ui::Progress& progress)
{
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    progress.append_to(list);
    size_t count = 0;
    for (const auto& batch : list.batches()) count += batch.vertices.size();
    return count;
}

void test_determinate_does_not_advance()
{
    ui::Progress progress(200.0f);
    progress.set_value(0.5f);
    progress.arrange({{0.0f, 0.0f}, {200.0f, 10.0f}});

    const float phase = progress.phase();
    progress.update(0.5f);
    assert(progress.phase() == phase);
    assert(!progress.indeterminate());
    assert(vertex_count(progress) > 0);
}

void test_indeterminate_advances_and_wraps()
{
    ui::Progress progress(200.0f);
    progress.set_indeterminate(true);
    progress.arrange({{0.0f, 0.0f}, {200.0f, 10.0f}});

    progress.update(0.3f); // 0.3 / 1.2 = 0.25
    assert(near(progress.phase(), 0.25f));

    progress.update(0.9f); // 1.0 -> 回绕到 0
    assert(near(progress.phase(), 0.0f));

    progress.update(1.8f); // 1.5 周期 -> 0.5
    assert(near(progress.phase(), 0.5f));
}

void test_indeterminate_band_moves()
{
    ui::Progress progress(200.0f);
    progress.set_indeterminate(true);
    progress.arrange({{0.0f, 0.0f}, {200.0f, 10.0f}});

    // 只采样 accent 色顶点（条带），跳过轨道与边框。
    const glm::vec4 accent = ui::Theme::dark().accent;
    const auto sample_band = [&]() {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        progress.append_to(list);
        float min_x = 1.0e9f;
        float max_x = -1.0e9f;
        for (const auto& batch : list.batches()) {
            for (const auto& vertex : batch.vertices) {
                if (std::fabs(vertex.color.r - accent.r) > 1e-3f) continue;
                if (std::fabs(vertex.color.g - accent.g) > 1e-3f) continue;
                min_x = std::min(min_x, vertex.pos.x);
                max_x = std::max(max_x, vertex.pos.x);
            }
        }
        return std::pair<float, float>{min_x, max_x};
    };

    progress.update(0.3f); // phase 0.25
    const auto early = sample_band();
    progress.update(0.6f); // phase 0.75
    const auto later = sample_band();

    // 条带整体右移，且始终 clamp 在控件范围内。
    assert(early.first < later.first);
    assert(early.second < later.second);
    assert(later.first >= 0.0f);
    assert(later.second <= 200.0f);

    // 接近右边界时宽度保持不变（靠裁剪滑出，而不是被压缩），
    // 否则看起来像“还没走出去就从头重来”。
    progress.update(0.18f); // phase 0.9
    const auto late = sample_band();
    assert(later.first < late.first);
    assert(std::fabs((late.second - late.first) - 60.0f) < 1.0f); // band_w = 200 * 0.3
    assert(late.second > 200.0f); // 超出右边界，由 scissor 裁掉
}

void test_switch_back_to_determinate()
{
    ui::Progress progress(200.0f);
    progress.set_indeterminate(true);
    progress.arrange({{0.0f, 0.0f}, {200.0f, 10.0f}});
    progress.update(0.6f);
    const float phase = progress.phase();

    progress.set_indeterminate(false);
    progress.update(0.6f);
    // 切回确定态后不再推进，但相位保留。
    assert(progress.phase() == phase);
    assert(vertex_count(progress) > 0);
}

} // namespace

int main()
{
    test_determinate_does_not_advance();
    test_indeterminate_advances_and_wraps();
    test_indeterminate_band_moves();
    test_switch_back_to_determinate();
    std::cout << "ui progress tests passed\n";
    return 0;
}
