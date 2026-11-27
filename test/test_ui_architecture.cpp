// UI-52：架构规则的守卫测试。
//
// 设计文档（.design/ui-redesign.md §3 / §6）把「依赖方向」和几条量化指标写成了契约。
// 没有断言守着，它们会在下一次改动里悄悄失效——本测试就是那道闸门。
//
// 检查方式：直接读源文件做文本断言（不引入额外工具链，符合 AGENTS 第 9 条）。
// 源码根目录由 CMake 通过 COSINE_SOURCE_DIR 传入。
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
int g_passed = 0;
int g_failed = 0;

void check(bool ok, const std::string& name, const std::string& detail = std::string())
{
    if (ok) {
        ++g_passed;
        std::printf("ok      %s\n", name.c_str());
        return;
    }
    ++g_failed;
    std::printf("FAIL    %s -- %s\n", name.c_str(), detail.c_str());
}

fs::path ui_root() { return fs::path(COSINE_SOURCE_DIR) / "src" / "ui"; }

std::string read_file(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

// 统计某个目录（递归）里所有头/源文件含指定片段的次数。
int count_occurrences(const fs::path& dir, const std::string& needle, const std::vector<std::string>& exts)
{
    int total = 0;
    if (!fs::exists(dir)) return total;
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        bool wanted = false;
        for (const auto& e : exts) wanted = wanted || (ext == e);
        if (!wanted) continue;
        const std::string text = read_file(entry.path());
        std::size_t pos = 0;
        while ((pos = text.find(needle, pos)) != std::string::npos) {
            ++total;
            pos += needle.size();
        }
    }
    return total;
}

// 逐行扫描：跳过纯注释行（避免把文档/注释里的文字算成违规）。
// 去掉行尾注释（处理 `code;  // 说明 children_ …` 这种情况）。
std::string strip_line_comment(const std::string& line)
{
    const std::size_t pos = line.find("//");
    return pos == std::string::npos ? line : line.substr(0, pos);
}

// 词边界匹配：避免 remove_children_if 这类标识符被当成 children_ 访问。
bool has_token(const std::string& line, const std::string& token)
{
    std::size_t pos = 0;
    while ((pos = line.find(token, pos)) != std::string::npos) {
        const bool left_ok = pos == 0 || !(std::isalnum(static_cast<unsigned char>(line[pos - 1]))
                                           || line[pos - 1] == '_');
        const std::size_t end = pos + token.size();
        const bool right_ok = end >= line.size()
            || !(std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_');
        if (left_ok && right_ok) return true;
        pos = end;
    }
    return false;
}

int count_code_lines_with(const fs::path& dir, const std::string& needle,
                          const std::vector<std::string>& exts)
{
    int total = 0;
    if (!fs::exists(dir)) return total;
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        bool wanted = false;
        for (const auto& e : exts) wanted = wanted || (ext == e);
        if (!wanted) continue;
        std::istringstream in(read_file(entry.path()));
        std::string line;
        while (std::getline(in, line)) {
            const std::string code = strip_line_comment(line);
            if (!has_token(code, needle)) continue;
            ++total;
        }
    }
    return total;
}
} // namespace

// D1：纯几何/布局工具层不得依赖控件树。
// 注：`layout_utils.hpp` 不在此列——它的签名就是「对 某个 Widget 求内容区」（
// content_rect(const Widget&) / place_positioned_children），属于「控件适配层」，
// 不是几何原语。真正该独立的原语是 rect / align / layout_metrics / layout_budget / measure_cache。
void test_geometry_does_not_depend_on_nodes()
{
    // UI-53 后纯原语住在 geom/（measure_cache 在 layout/）。
    // 注意：文件不存在时必须判失败——否则路径写错会让断言“真空通过”。
    const fs::path geom = ui_root() / "geom";
    for (const char* file : {"align.hpp", "rect.hpp", "layout_metrics.hpp", "layout_budget.hpp"}) {
        const fs::path p = geom / file;
        if (!fs::exists(p)) {
            check(false, std::string("D1: geom/") + file + " exists");
            continue;
        }
        const std::string text = read_file(p);
        check(!contains(text, "#include \"ui/node/widget.hpp\"")
                  && !contains(text, "#include \"ui/widget.hpp\""),
              std::string("D1: geom/") + file + " does not include the widget tree");
    }
    const fs::path cache = ui_root() / "layout" / "measure_cache.hpp";
    check(fs::exists(cache), "D1: layout/measure_cache.hpp exists");
    if (fs::exists(cache)) {
        const std::string text = read_file(cache);
        check(!contains(text, "#include \"ui/node/widget.hpp\""),
              "D1: layout/measure_cache.hpp does not include the widget tree");
    }
}

// D3：样式层不得依赖具体控件。
void test_style_does_not_depend_on_widgets()
{
    // UI-53 后样式层住在 ui/style/；逐个文件检查不得 include 具体控件。
    const fs::path style = ui_root() / "style";
    if (!fs::exists(style)) {
        check(false, "D3: ui/style/ exists");
        return;
    }
    for (const auto& entry : fs::directory_iterator(style)) {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        if (ext != ".hpp" && ext != ".cpp") continue;
        const std::string text = read_file(entry.path());
        const bool bad = contains(text, "#include \"ui/core/")
            || contains(text, "#include \"ui/containers/")
            || contains(text, "#include \"ui/composites/")
            || contains(text, "#include \"ui/views/");
        check(!bad, "D3: style/" + entry.path().filename().string()
                        + " does not include concrete widgets");
    }
}

// D5（UI-53）：旧路径只能留“转发头”，不得再放实现。
// 这样兼容层不会变成第二套真相，也为后续删除兼容头提供了硬条件。
void test_shims_are_trivial()
{
    int shims = 0;
    int bad = 0;
    for (const auto& entry : fs::directory_iterator(ui_root())) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".hpp") continue;
        const std::string text = read_file(entry.path());
        if (!contains(text, "兼容转发（UI-53）")) continue;
        ++shims;
        int code_lines = 0;
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line)) {
            const std::size_t first = line.find_first_not_of(" \t");
            if (first == std::string::npos) continue;
            if (line.compare(first, 15, "#pragma once") == 0) continue;
            if (line.compare(first, 2, "//") == 0) continue;
            ++code_lines;
        }
        if (code_lines > 1) ++bad;
    }
    check(shims == 22, "D5: expected 22 forwarding shims", "found=" + std::to_string(shims));
    check(bad == 0, "D5: shims contain only a single #include line",
          "bad=" + std::to_string(bad));
}

// D2：基础控件与容器不得反向依赖复合控件/视图。
void test_lower_layers_do_not_include_upper_layers()
{
    const std::vector<std::string> exts{".hpp", ".cpp"};
    const int core_up = count_occurrences(ui_root() / "core", "#include \"ui/composites/", exts)
        + count_occurrences(ui_root() / "core", "#include \"ui/views/", exts);
    check(core_up == 0, "D2: ui/core does not include composites/views",
          "hits=" + std::to_string(core_up));

    const int containers_up = count_occurrences(ui_root() / "containers", "#include \"ui/composites/", exts)
        + count_occurrences(ui_root() / "containers", "#include \"ui/views/", exts);
    check(containers_up == 0, "D2: ui/containers does not include composites/views",
          "hits=" + std::to_string(containers_up));
}

// K2：树存储私有——派生类不得直接读写 children_（widget.hpp 是唯一合法使用者）。
void test_tree_storage_is_private()
{
    const std::vector<std::string> exts{".hpp", ".cpp"};
    int outside = 0;
    int inside = 0;
    for (const auto& entry : fs::recursive_directory_iterator(ui_root())) {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        if (ext != ".hpp" && ext != ".cpp") continue;
        const bool is_self = entry.path().filename() == "widget.hpp";
        std::istringstream in(read_file(entry.path()));
        std::string line;
        while (std::getline(in, line)) {
            if (!has_token(strip_line_comment(line), "children_")) continue;
            if (is_self) ++inside;
            else ++outside;
        }
    }
    check(inside > 0, "K2: widget.hpp owns the child storage");
    check(outside == 0, "K2: no children_ access outside widget.hpp",
          "hits=" + std::to_string(outside));
    (void)exts;
}

// K3：几何只能经 set_geometry 写（写点全部在 widget.hpp 的入口函数里）。
void test_geometry_single_write_entry()
{
    const std::vector<std::string> exts{".hpp", ".cpp"};
    int outside = 0;
    for (const char* needle : {"global_ =", "global_.pos =", "global_.size =",
                               "local_ =", "local_.pos =", "local_.size ="}) {
        for (const auto& entry : fs::recursive_directory_iterator(ui_root())) {
            if (!entry.is_regular_file()) continue;
            const auto ext = entry.path().extension().string();
            if (ext != ".hpp" && ext != ".cpp") continue;
            if (entry.path().filename() == "widget.hpp") continue;
            std::istringstream in(read_file(entry.path()));
            std::string line;
            while (std::getline(in, line)) {
                const std::string code = strip_line_comment(line);
                if (code.find(needle) != std::string::npos) ++outside;
            }
        }
    }
    check(outside == 0, "K3: geometry writes only inside set_geometry family",
          "outside_hits=" + std::to_string(outside));
    (void)exts;
}

// K5：基类虚函数数量上限（UI-47 之后 34，UI-52 增加语义钩子后 39）。
void test_base_virtual_budget()
{
    const std::string text = read_file(ui_root() / "widget.hpp");
    int virtuals = 0;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        if (line.compare(first, 8, "virtual ") == 0) ++virtuals;
    }
    check(virtuals <= 42, "K5: Widget virtual budget (<= 42)",
          "virtuals=" + std::to_string(virtuals));
}

// K1/K6：状态与缓存各只有一处。
void test_state_and_cache_ownership()
{
    const std::vector<std::string> exts{".hpp", ".cpp"};
    check(count_code_lines_with(ui_root(), "bool enabled_", exts) == 0,
          "K1: no per-widget enabled_ field");
    check(count_code_lines_with(ui_root(), "bool pressed_", exts) == 0,
          "K1: no per-widget pressed_ field");
    check(count_code_lines_with(ui_root(), "bool dragging_", exts) == 0,
          "K1: no per-widget dragging_ field");
    check(count_code_lines_with(ui_root(), "control_state(", exts) == 0,
          "K1: control_state() free function is gone (use Interaction::state)");
    check(count_code_lines_with(ui_root(), "struct MeasureKey", exts) == 1,
          "K6: exactly one MeasureKey definition (measure_cache.hpp)");
}

int main()
{
    test_geometry_does_not_depend_on_nodes();
    test_style_does_not_depend_on_widgets();
    test_shims_are_trivial();
    test_lower_layers_do_not_include_upper_layers();
    test_tree_storage_is_private();
    test_geometry_single_write_entry();
    test_base_virtual_budget();
    test_state_and_cache_ownership();

    std::printf("architecture: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
