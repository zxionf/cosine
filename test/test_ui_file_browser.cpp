#include "ui/views/file_browser.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace
{
using xel::ui::FileBrowser;

// 造一棵固定目录树：sub_a/ sub_b/ alpha.cos zeta.cos notes.txt
fs::path make_fixture()
{
    const fs::path dir = fs::temp_directory_path() / "cosine_file_browser_fixture";
    fs::remove_all(dir);
    fs::create_directories(dir / "sub_b");
    fs::create_directories(dir / "sub_a");
    std::ofstream(dir / "zeta.cos") << "z";
    std::ofstream(dir / "alpha.cos") << "a";
    std::ofstream(dir / "notes.txt") << "n";
    return dir;
}

// 目录在前、各自按名字升序；扩展名过滤只作用于文件。
void test_ordering_and_filter()
{
    const fs::path dir = make_fixture();
    FileBrowser browser;
    browser.set_extension_filter(".cos");
    browser.set_directory(dir.string());

    assert(browser.entry_count() == 5);
    assert(browser.entry_name(0) == ".." && browser.entry_is_directory(0));
    assert(browser.entry_name(1) == "sub_a" && browser.entry_is_directory(1));
    assert(browser.entry_name(2) == "sub_b" && browser.entry_is_directory(2));
    assert(browser.entry_name(3) == "alpha.cos" && !browser.entry_is_directory(3));
    assert(browser.entry_name(4) == "zeta.cos" && !browser.entry_is_directory(4));

    // 去掉过滤后 .txt 也可见。
    browser.set_extension_filter("");
    assert(browser.entry_count() == 6);

    fs::remove_all(dir);
}

// 单击选中并回调；双击文件触发 activate，双击目录只换目录。
void test_click_and_activate()
{
    const fs::path dir = make_fixture();
    FileBrowser browser(glm::vec2{400.0f, 300.0f});
    browser.set_row_height(30.0f);
    browser.set_extension_filter(".cos");
    browser.set_directory(dir.string());
    browser.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});

    int selects = 0;
    int activates = 0;
    browser.set_on_select([&selects] { ++selects; });
    browser.set_on_activate([&activates] { ++activates; });

    // 第 3 行（0 基）是 alpha.cos。
    const float file_y = 3.0f * 30.0f + 15.0f;
    assert(browser.on_mouse_down({20.0f, file_y}, 0));
    assert(selects == 1);
    assert(browser.selected_name() == "alpha.cos");
    assert(!browser.selected_is_directory());
    assert(browser.selected_path() == (dir / "alpha.cos").string());

    assert(browser.on_double_click({20.0f, file_y}, 0));
    assert(activates == 1);

    // 双击目录 sub_a：进入，目录变化回调。
    int directory_changes = 0;
    browser.set_on_directory_changed([&directory_changes] { ++directory_changes; });
    const float dir_y = 1.0f * 30.0f + 15.0f;
    assert(browser.on_double_click({20.0f, dir_y}, 0));
    assert(activates == 1);
    assert(directory_changes == 1);
    assert(browser.directory() == (dir / "sub_a").string());

    // 在空目录里双击 ".." 回到上级。
    assert(browser.entry_name(0) == "..");
    assert(browser.on_double_click({20.0f, 15.0f}, 0));
    assert(browser.directory() == dir.string());

    fs::remove_all(dir);
}

// 好路径拼接：相对名基于当前目录，绝对名原样返回。
void test_resolve()
{
    const fs::path dir = make_fixture();
    FileBrowser browser;
    browser.set_directory(dir.string());

    assert(browser.resolve("a.cos") == (dir / "a.cos").string());
    const fs::path absolute = fs::temp_directory_path() / "cosine_abs_probe.cos";
    assert(browser.resolve(absolute.string()) == absolute.string());
    assert(browser.resolve("").empty());

    fs::remove_all(dir);
}

// 行高不足时内容可滚动，且滚出边界被夹回。
void test_scroll_clamped()
{
    const fs::path dir = make_fixture();
    FileBrowser browser(glm::vec2{400.0f, 60.0f});
    browser.set_row_height(30.0f);
    browser.set_extension_filter(".cos");
    browser.set_directory(dir.string());
    browser.arrange({{0.0f, 0.0f}, {400.0f, 60.0f}});
    assert(browser.entry_count() == 5);

    assert(browser.on_scroll({10.0f, 10.0f}, {0.0f, 5.0f}));
    assert(browser.on_scroll({10.0f, 10.0f}, {0.0f, 500.0f})); // 远超内容
    // 再往回滚远超，偏移被夹到 0。
    assert(browser.on_scroll({10.0f, 10.0f}, {0.0f, -500.0f}));

    // 高度足够时不吞滚轮。
    FileBrowser tall(glm::vec2{400.0f, 400.0f});
    tall.set_directory(dir.string());
    tall.arrange({{0.0f, 0.0f}, {400.0f, 400.0f}});
    assert(!tall.on_scroll({10.0f, 10.0f}, {0.0f, 3.0f}));

    fs::remove_all(dir);
}
} // namespace

int main()
{
    test_ordering_and_filter();
    test_click_and_activate();
    test_resolve();
    test_scroll_clamped();
    std::printf("test_ui_file_browser: all passed\n");
    return 0;
}
