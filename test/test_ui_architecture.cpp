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
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "ui/node/layout_node.hpp"
#include "ui/node/node.hpp"
#include "ui/node/widget.hpp"

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

// D5（UI-54/R8）：旧路径已彻底清除——src/ui 根目录不再有头文件，也没有转发头。
// 保留这条断言是因为：一旦有人为了“方便”再建一个旧路径转发头，它会立即红。
void test_shims_are_gone()
{
    int root_headers = 0;
    int shims = 0;
    for (const auto& entry : fs::directory_iterator(ui_root())) {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        if (ext == ".hpp" || ext == ".cpp") ++root_headers;
        if (ext != ".hpp") continue;
        if (contains(read_file(entry.path()), "兼容转发")) ++shims;
    }
    check(root_headers == 0, "D5: src/ui root has no headers/sources (all moved into subdirs)",
          "found=" + std::to_string(root_headers));
    check(shims == 0, "D5: no forwarding shims left", "found=" + std::to_string(shims));
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

// K2：树存储私有——只有 Node 的实现（node/node.hpp + node/node.cpp）能碰 children_。
void test_tree_storage_is_private()
{
    int outside = 0;
    int inside = 0;
    for (const auto& entry : fs::recursive_directory_iterator(ui_root())) {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        if (ext != ".hpp" && ext != ".cpp") continue;
        const std::string name = entry.path().filename().string();
        const bool is_node_module = entry.path().parent_path().filename() == "node"
            && (name == "node.hpp" || name == "node.cpp");
        std::istringstream in(read_file(entry.path()));
        std::string line;
        while (std::getline(in, line)) {
            if (!has_token(strip_line_comment(line), "children_")) continue;
            if (is_node_module) ++inside;
            else ++outside;
        }
    }
    check(inside > 0, "K2: node/ (node.hpp + node.cpp) owns the child storage");
    check(outside == 0, "K2: no children_ access outside the node module",
          "hits=" + std::to_string(outside));
}

// K3：几何只能经 set_geometry 写（写点全部在 node/node.hpp 的入口函数里）。
void test_geometry_single_write_entry()
{
    int outside = 0;
    for (const char* needle : {"global_ =", "global_.pos =", "global_.size =",
                               "local_ =", "local_.pos =", "local_.size ="}) {
        for (const auto& entry : fs::recursive_directory_iterator(ui_root())) {
            if (!entry.is_regular_file()) continue;
            const auto ext = entry.path().extension().string();
            if (ext != ".hpp" && ext != ".cpp") continue;
            if (entry.path().filename() == "node.hpp") continue;
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
}

// K5：基类虚函数数量上限（UI-56 分层后：Node + LayoutNode 合计）。
void test_base_virtual_budget()
{
    int virtuals = 0;
    int on_hooks = 0;
    for (const char* file : {"node/node.hpp", "node/layout_node.hpp"}) {
        std::istringstream in(read_file(ui_root() / file));
        std::string line;
        while (std::getline(in, line)) {
            const std::size_t first = line.find_first_not_of(" \t");
            if (first == std::string::npos) continue;
            if (line.compare(first, 8, "virtual ") != 0) continue;
            ++virtuals;
            if (line.find(" on_") != std::string::npos) ++on_hooks;
        }
    }
    check(virtuals <= 45, "K5: Node+LayoutNode virtual budget (<= 45)",
          "virtuals=" + std::to_string(virtuals));
    check(on_hooks == 17, "K5: the 17 event hooks still live in LayoutNode",
          "hooks=" + std::to_string(on_hooks));
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

// K7（UI-37/UI-56）：布局意图的写入点收敛——`layout_.` 的直接赋值只能出现在
// set_layout_spec()（批量）与两处 absolute_pos 写入（set_absolute / set_absolute_pos）。
// 头文件 11 处 + 实现文件 1 处 = 12；新增第三个写入者时这个数字会变。
void test_layout_single_writer()
{
    int writes = 0;
    for (const char* file : {"node/layout_node.hpp", "node/layout_node.cpp"}) {
        const std::string text = read_file(ui_root() / file);
        for (const char* field : {"align_self", "has_align", "flex_grow", "size_percent",
                                  "aspect_ratio", "fill_x", "fill_y", "min_size", "max_size",
                                  "preferred", "absolute_pos"}) {
            const std::string needle = std::string("layout_.") + field + " =";
            std::size_t pos = 0;
            while ((pos = text.find(needle, pos)) != std::string::npos) {
                const std::size_t after = pos + needle.size();
                // 排除 `layout_.display == …` 这类比较表达式。
                if (after < text.size() && text[after] != '=') ++writes;
                pos = after;
            }
        }
    }
    check(writes == 12, "K7: layout_ is written by exactly two functions (12 assignments)",
          "found=" + std::to_string(writes));
}

// K8（UI-56 + 节点改造）：两层结构成立——Node ⊂ LayoutNode，Widget 是 LayoutNode 的别名；
// 「容纳子节点」直接在 Node 上抽象（add → adopt），LeafNode / ContainerNode 已删除。
void test_three_layer_structure()
{
    static_assert(std::is_base_of_v<xel::ui::Node, xel::ui::LayoutNode>, "LayoutNode must derive from Node");
    static_assert(std::is_same_v<xel::ui::Widget, xel::ui::LayoutNode>, "Widget is an alias of LayoutNode");
    static_assert(!std::is_same_v<xel::ui::Node, xel::ui::LayoutNode>, "Node and LayoutNode are distinct");

    check(sizeof(xel::ui::Node) < sizeof(xel::ui::LayoutNode), "Node is lighter than LayoutNode",
          "node=" + std::to_string(sizeof(xel::ui::Node))
              + " layout=" + std::to_string(sizeof(xel::ui::LayoutNode)));
    // 树的元素类型是 Node（不是 Widget）——这是分层的核心事实。
    check(sizeof(decltype(std::declval<const xel::ui::Node&>().children()))
              == sizeof(std::vector<std::unique_ptr<xel::ui::Node>>),
          "children() stores Node, not Widget");
    check(fs::exists(ui_root() / "node" / "node.hpp") && fs::exists(ui_root() / "node" / "layout_node.hpp"),
          "two-layer headers exist (node.hpp / layout_node.hpp)");

    // 中间基类不许复活：头文件不存在，源码里也没有引用。
    check(!fs::exists(ui_root() / "node" / "leaf_node.hpp")
              && !fs::exists(ui_root() / "node" / "container_node.hpp"),
          "K8: leaf_node.hpp / container_node.hpp are gone");
    const std::vector<std::string> exts{".hpp", ".cpp"};
    const int legacy = count_occurrences(ui_root(), "LeafNode", exts)
        + count_occurrences(ui_root(), "ContainerNode", exts);
    check(legacy == 0, "K8: no LeafNode / ContainerNode references in src/ui",
          "hits=" + std::to_string(legacy));

    // 容纳子节点的能力在 Node：add() 默认转发到 adopt()。
    const std::string node_text = read_file(ui_root() / "node" / "node.hpp");
    check(contains(node_text, "void add(std::unique_ptr<Node> child) { adopt(std::move(child)); }"),
          "K8: Node::add defaults to adopt() (containment lives on Node)");
}

int main()
{
    test_geometry_does_not_depend_on_nodes();
    test_style_does_not_depend_on_widgets();
    test_shims_are_gone();
    test_lower_layers_do_not_include_upper_layers();
    test_tree_storage_is_private();
    test_geometry_single_write_entry();
    test_base_virtual_budget();
    test_state_and_cache_ownership();
    test_layout_single_writer();
    test_three_layer_structure();

    std::printf("architecture: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
