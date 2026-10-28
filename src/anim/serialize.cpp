#include "serialize.hpp"

#include "easing.hpp"
#include "property.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace xel::anim
{
namespace
{
// 文件头常量，见 docs/project_format.md。
constexpr std::uint8_t kMagic[4] = {'C', 'S', 'N', 'P'};
constexpr std::uint8_t kGuard[4] = {0x0D, 0x0A, 0x1A, 0x0A};
constexpr std::uint32_t kHeaderSize = 16;
constexpr std::uint32_t kEndMarker = 0x21444E45; // 字节 'E' 'N' 'D' '!'

constexpr std::uint32_t kMaxStringBytes = 64u * 1024u * 1024u;
constexpr std::uint32_t kMaxNodes = 1'000'000u;
constexpr std::uint32_t kMaxKeys = 1'000'000u;

void put_u8(std::vector<std::uint8_t>& out, std::uint8_t v)
{
    out.push_back(v);
}

void put_u16(std::vector<std::uint8_t>& out, std::uint16_t v)
{
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v)
{
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

void put_u64(std::vector<std::uint8_t>& out, std::uint64_t v)
{
    put_u32(out, static_cast<std::uint32_t>(v & 0xFFFFFFFFu));
    put_u32(out, static_cast<std::uint32_t>((v >> 32) & 0xFFFFFFFFu));
}

void put_f32(std::vector<std::uint8_t>& out, float v)
{
    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(v));
    std::memcpy(&bits, &v, sizeof(bits));
    put_u32(out, bits);
}

void put_str(std::vector<std::uint8_t>& out, const std::string& s)
{
    put_u32(out, static_cast<std::uint32_t>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

void put_vec2(std::vector<std::uint8_t>& out, const glm::vec2& v)
{
    put_f32(out, v.x);
    put_f32(out, v.y);
}

void put_vec4(std::vector<std::uint8_t>& out, const glm::vec4& v)
{
    put_f32(out, v.x);
    put_f32(out, v.y);
    put_f32(out, v.z);
    put_f32(out, v.w);
}

// value_tag 与 docs/project_format.md §4 保持一致；不使用 variant::index()。
void put_value(std::vector<std::uint8_t>& out, const PropertyValue& value)
{
    std::visit(
        [&out](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, float>) {
                put_u8(out, 0);
                put_f32(out, v);
            } else if constexpr (std::is_same_v<T, glm::vec2>) {
                put_u8(out, 1);
                put_vec2(out, v);
            } else if constexpr (std::is_same_v<T, glm::vec3>) {
                put_u8(out, 2);
                put_f32(out, v.x);
                put_f32(out, v.y);
                put_f32(out, v.z);
            } else if constexpr (std::is_same_v<T, glm::vec4>) {
                put_u8(out, 3);
                put_vec4(out, v);
            } else if constexpr (std::is_same_v<T, bool>) {
                put_u8(out, 4);
                put_u8(out, v ? 1 : 0);
            } else if constexpr (std::is_same_v<T, std::string>) {
                put_u8(out, 5);
                put_str(out, v);
            } else if constexpr (std::is_same_v<T, std::uint32_t>) {
                put_u8(out, 6);
                put_u32(out, v);
            }
        },
        value);
}

struct Reader
{
    std::span<const std::uint8_t> bytes;
    std::size_t offset = 0;
    std::string error;

    bool ok() const { return error.empty(); }

    bool fail(std::string message)
    {
        if (error.empty()) error = std::move(message);
        return false;
    }

    bool need(std::size_t n)
    {
        if (offset + n > bytes.size()) {
            return fail("unexpected end of file at offset " + std::to_string(offset));
        }
        return true;
    }

    bool u8(std::uint8_t& v)
    {
        if (!need(1)) return false;
        v = bytes[offset++];
        return true;
    }

    bool u16(std::uint16_t& v)
    {
        if (!need(2)) return false;
        v = static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
        offset += 2;
        return true;
    }

    bool u32(std::uint32_t& v)
    {
        if (!need(4)) return false;
        v = static_cast<std::uint32_t>(bytes[offset])
          | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
          | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
          | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
        offset += 4;
        return true;
    }

    bool u64(std::uint64_t& v)
    {
        std::uint32_t lo = 0;
        std::uint32_t hi = 0;
        if (!u32(lo) || !u32(hi)) return false;
        v = (static_cast<std::uint64_t>(hi) << 32) | lo;
        return true;
    }

    bool f32(float& v)
    {
        std::uint32_t bits = 0;
        if (!u32(bits)) return false;
        std::memcpy(&v, &bits, sizeof(v));
        return true;
    }

    bool str(std::string& out)
    {
        std::uint32_t length = 0;
        if (!u32(length)) return false;
        if (length > kMaxStringBytes) {
            return fail("length " + std::to_string(length) + " exceeds limit");
        }
        if (!need(length)) return false;
        out.assign(reinterpret_cast<const char*>(bytes.data() + offset), length);
        offset += length;
        return true;
    }

    bool vec2(glm::vec2& v) { return f32(v.x) && f32(v.y); }
    bool vec4(glm::vec4& v) { return f32(v.x) && f32(v.y) && f32(v.z) && f32(v.w); }

    bool value(PropertyValue& out)
    {
        std::uint8_t tag = 0;
        if (!u8(tag)) return false;
        switch (tag) {
            case 0: {
                float v = 0.0f;
                if (!f32(v)) return false;
                out = v;
                return true;
            }
            case 1: {
                glm::vec2 v{};
                if (!vec2(v)) return false;
                out = v;
                return true;
            }
            case 2: {
                glm::vec3 v{};
                if (!f32(v.x) || !f32(v.y) || !f32(v.z)) return false;
                out = v;
                return true;
            }
            case 3: {
                glm::vec4 v{};
                if (!vec4(v)) return false;
                out = v;
                return true;
            }
            case 4: {
                std::uint8_t v = 0;
                if (!u8(v)) return false;
                out = (v != 0);
                return true;
            }
            case 5: {
                std::string v;
                if (!str(v)) return false;
                out = std::move(v);
                return true;
            }
            case 6: {
                std::uint32_t v = 0;
                if (!u32(v)) return false;
                out = v;
                return true;
            }
            default:
                return fail("unknown value tag " + std::to_string(tag));
        }
    }
};

bool is_known_node_type(std::uint8_t type)
{
    return type <= static_cast<std::uint8_t>(scene::NodeType::Camera);
}

bool is_known_easing(std::uint8_t easing)
{
    return easing <= static_cast<std::uint8_t>(Easing::CubicBezier);
}
} // namespace

std::vector<std::uint8_t> serialize(const Project& project)
{
    std::vector<std::uint8_t> out;
    out.reserve(1024);

    // 头部 16 字节。
    out.insert(out.end(), std::begin(kMagic), std::end(kMagic));
    out.insert(out.end(), std::begin(kGuard), std::end(kGuard));
    put_u8(out, kFormatMajor);
    put_u8(out, kFormatMinor);
    put_u16(out, 0); // flags：全部保留位为 0
    put_u32(out, kHeaderSize);

    // 节点表：按 id 升序，保证确定性。
    std::vector<const scene::Node*> nodes;
    nodes.reserve(project.scene.size());
    for (const auto& entry : project.scene.nodes()) nodes.push_back(&entry.second);
    std::sort(nodes.begin(), nodes.end(),
              [](const scene::Node* a, const scene::Node* b) { return a->id < b->id; });

    put_u32(out, static_cast<std::uint32_t>(nodes.size()));
    for (const scene::Node* node : nodes) {
        put_u64(out, node->id);
        put_u8(out, static_cast<std::uint8_t>(node->type));

        std::uint8_t flags = 0;
        if (node->visible) flags |= 0x01;
        if (node->ellipse) flags |= 0x02;
        put_u8(out, flags);

        put_vec2(out, node->transform.position);
        put_f32(out, node->transform.rotation);
        put_vec2(out, node->transform.scale);
        put_vec2(out, node->size);
        put_vec4(out, node->color);
        put_f32(out, node->text_size);
        put_u32(out, node->texture_id);
        put_f32(out, node->zoom);
        put_str(out, node->name);
        put_str(out, node->text);
        // 1.1 新增：图片资源路径（稳定标识，texture_id 只是运行时槽号）。
        put_str(out, node->texture_path);

        put_u32(out, static_cast<std::uint32_t>(node->children.size()));
        for (const scene::NodeId child : node->children) put_u64(out, child);
    }

    // 时间轴元数据。
    put_f32(out, project.timeline.duration());
    put_f32(out, project.timeline.fps());
    put_u8(out, project.timeline.loop() ? 1 : 0);
    put_f32(out, project.timeline.time_scale());

    // 轨道表：按 (node, property) 升序。
    std::vector<const Track*> tracks;
    tracks.reserve(project.timeline.tracks().size());
    for (const Track& track : project.timeline.tracks()) tracks.push_back(&track);
    std::sort(tracks.begin(), tracks.end(), [](const Track* a, const Track* b) {
        if (a->node() != b->node()) return a->node() < b->node();
        return a->property() < b->property();
    });

    put_u32(out, static_cast<std::uint32_t>(tracks.size()));
    for (const Track* track : tracks) {
        put_u64(out, track->node());
        put_u32(out, track->property());
        put_u32(out, static_cast<std::uint32_t>(track->keys().size()));
        for (const Keyframe& key : track->keys()) {
            put_f32(out, key.time);
            put_u8(out, static_cast<std::uint8_t>(key.easing));
            put_f32(out, key.custom.x1);
            put_f32(out, key.custom.y1);
            put_f32(out, key.custom.x2);
            put_f32(out, key.custom.y2);
            put_value(out, key.value);
        }
    }

    put_u32(out, kEndMarker);
    return out;
}

bool deserialize(std::span<const std::uint8_t> bytes, Project& out, std::string* error)
{
    Reader reader{bytes, 0, {}};
    auto finish = [&](bool result) {
        if (!result && error != nullptr && error->empty()) *error = reader.error;
        return result;
    };

    // 魔术头与 guard。
    if (!reader.need(8)) return finish(false);
    if (std::memcmp(bytes.data(), kMagic, 4) != 0) {
        reader.error = "not a cosine project (bad magic)";
        return finish(false);
    }
    if (std::memcmp(bytes.data() + 4, kGuard, 4) != 0) {
        reader.error = "project file was corrupted by text-mode transfer";
        return finish(false);
    }
    reader.offset = 8;

    std::uint8_t major = 0;
    std::uint8_t minor = 0;
    std::uint16_t flags = 0;
    std::uint32_t header_size = 0;
    if (!reader.u8(major) || !reader.u8(minor) || !reader.u16(flags) || !reader.u32(header_size)) {
        return finish(false);
    }

    if (major != kFormatMajor) {
        reader.error = "unsupported project version " + std::to_string(major) + "."
                     + std::to_string(minor) + " (this build supports "
                     + std::to_string(kFormatMajor) + ".x)";
        return finish(false);
    }
    if (minor > kFormatMinor) {
        reader.error = "project file is newer than this build: version " + std::to_string(major)
                     + "." + std::to_string(minor) + " > " + std::to_string(kFormatMajor) + "."
                     + std::to_string(kFormatMinor);
        return finish(false);
    }
    if (header_size < 16 || header_size > bytes.size()) {
        reader.error = "invalid header size " + std::to_string(header_size);
        return finish(false);
    }
    // 头部可能有未来扩展字段：按 header_size 跳过。
    reader.offset = header_size;
    (void)flags;

    Project project;

    // 节点表。
    std::uint32_t node_count = 0;
    if (!reader.u32(node_count)) return finish(false);
    if (node_count > kMaxNodes) {
        reader.error = "node count " + std::to_string(node_count) + " exceeds limit";
        return finish(false);
    }

    std::vector<scene::Node> loaded;
    loaded.reserve(node_count);
    for (std::uint32_t i = 0; i < node_count; ++i) {
        scene::Node node;
        std::uint8_t type = 0;
        std::uint8_t node_flags = 0;
        std::uint32_t child_count = 0;

        if (!reader.u64(node.id) || !reader.u8(type) || !reader.u8(node_flags)
            || !reader.vec2(node.transform.position) || !reader.f32(node.transform.rotation)
            || !reader.vec2(node.transform.scale) || !reader.vec2(node.size)
            || !reader.vec4(node.color) || !reader.f32(node.text_size)
            || !reader.u32(node.texture_id) || !reader.f32(node.zoom)
            || !reader.str(node.name) || !reader.str(node.text)) {
            return finish(false);
        }
        // 1.1 起有 texture_path；读 1.0 文件时留空，重载后由编辑器按需重新解析。
        if (minor >= 1 && !reader.str(node.texture_path)) return finish(false);

        if (!is_known_node_type(type)) {
            reader.error = "unknown node type " + std::to_string(type);
            return finish(false);
        }
        node.type = static_cast<scene::NodeType>(type);
        node.visible = (node_flags & 0x01) != 0;
        node.ellipse = (node_flags & 0x02) != 0;
        node.parent = scene::kInvalidNode;

        if (!reader.u32(child_count) || child_count > node_count) {
            if (reader.ok()) {
                reader.error = "invalid child count " + std::to_string(child_count);
            }
            return finish(false);
        }
        node.children.resize(child_count);
        for (std::uint32_t c = 0; c < child_count; ++c) {
            if (!reader.u64(node.children[c])) return finish(false);
        }

        loaded.push_back(std::move(node));
    }

    // 时间轴元数据。
    float duration = 0.0f;
    float fps = 0.0f;
    std::uint8_t loop = 0;
    float time_scale = 1.0f;
    if (!reader.f32(duration) || !reader.f32(fps) || !reader.u8(loop) || !reader.f32(time_scale)) {
        return finish(false);
    }

    // 轨道表。
    std::uint32_t track_count = 0;
    if (!reader.u32(track_count)) return finish(false);
    if (track_count > kMaxNodes) {
        reader.error = "track count " + std::to_string(track_count) + " exceeds limit";
        return finish(false);
    }

    struct LoadedKey
    {
        std::uint64_t node = 0;
        std::uint32_t property = 0;
        Keyframe key{};
    };
    std::vector<LoadedKey> loaded_keys;
    for (std::uint32_t t = 0; t < track_count; ++t) {
        std::uint64_t node_id = 0;
        std::uint32_t property = 0;
        std::uint32_t key_count = 0;
        if (!reader.u64(node_id) || !reader.u32(property) || !reader.u32(key_count)) {
            return finish(false);
        }
        if (key_count > kMaxKeys) {
            reader.error = "keyframe count " + std::to_string(key_count) + " exceeds limit";
            return finish(false);
        }

        for (std::uint32_t k = 0; k < key_count; ++k) {
            LoadedKey item;
            item.node = node_id;
            item.property = property;

            std::uint8_t easing = 0;
            if (!reader.f32(item.key.time) || !reader.u8(easing)
                || !reader.f32(item.key.custom.x1) || !reader.f32(item.key.custom.y1)
                || !reader.f32(item.key.custom.x2) || !reader.f32(item.key.custom.y2)
                || !reader.value(item.key.value)) {
                return finish(false);
            }
            if (!is_known_easing(easing)) {
                reader.error = "unknown easing " + std::to_string(easing);
                return finish(false);
            }
            item.key.easing = static_cast<Easing>(easing);

            loaded_keys.push_back(std::move(item));
        }
    }

    std::uint32_t end_marker = 0;
    if (!reader.u32(end_marker)) return finish(false);
    if (end_marker != kEndMarker) {
        reader.error = "missing end marker";
        return finish(false);
    }

    // ---- 还原场景 ----
    // 先清掉 Scene 构造时自建的 root（id=1），完全按文件内容重建。
    project.scene.clear();

    // 注意传副本：后面第二遍还要读 loaded[i].children 重建父子关系。
    for (const scene::Node& node : loaded) {
        if (project.scene.insert(node) == scene::kInvalidNode) {
            reader.error = "duplicate or invalid node id";
            return finish(false);
        }
    }

    // 第二遍：按 child_ids 重建父子关系，并校验多重父与成环。
    std::unordered_map<scene::NodeId, scene::NodeId> parent_of;
    for (const scene::Node& node : loaded) {
        for (const scene::NodeId child : node.children) {
            if (project.scene.find(child) == nullptr) {
                reader.error = "invalid hierarchy: unknown child " + std::to_string(child);
                return finish(false);
            }
            if (parent_of.count(child) != 0) {
                reader.error = "invalid hierarchy: node " + std::to_string(child)
                             + " has multiple parents";
                return finish(false);
            }
            parent_of.emplace(child, node.id);
            if (auto* target = project.scene.find(child)) target->parent = node.id;
        }
    }

    // 成环检测：沿 parent 上溯步数不超过节点数。
    const std::size_t node_total = project.scene.size();
    for (const scene::Node& node : loaded) {
        std::size_t steps = 0;
        scene::NodeId cur = node.id;
        while (true) {
            const auto it = parent_of.find(cur);
            if (it == parent_of.end()) break;
            cur = it->second;
            if (++steps > node_total) {
                reader.error = "invalid hierarchy: cycle at node " + std::to_string(node.id);
                return finish(false);
            }
        }
    }

    // root：没有被任何节点引用为子节点、且 id 最小的那个。
    {
        scene::NodeId root_id = scene::kInvalidNode;
        for (const scene::Node& node : loaded) {
            if (parent_of.count(node.id) != 0) continue;
            if (root_id == scene::kInvalidNode || node.id < root_id) root_id = node.id;
        }
        if (root_id != scene::kInvalidNode) project.scene.set_root(root_id);
    }

    // ---- 还原时间轴 ----
    project.timeline.set_duration(duration);
    project.timeline.set_fps(fps);
    project.timeline.set_loop(loop != 0);
    project.timeline.set_time_scale(time_scale);

    for (LoadedKey& item : loaded_keys) {
        if (project.scene.find(static_cast<scene::NodeId>(item.node)) == nullptr) {
            reader.error = "track references unknown node " + std::to_string(item.node);
            return finish(false);
        }
        Track& track = project.timeline.ensure_track(static_cast<scene::NodeId>(item.node),
                                                     item.property);
        track.add_key(std::move(item.key));
    }

    out = std::move(project);
    if (error != nullptr) error->clear();
    return true;
}

bool save_project_file(const std::string& path, const Project& project, std::string* error)
{
    const std::vector<std::uint8_t> bytes = serialize(project);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        if (error != nullptr) *error = "cannot open for writing: " + path;
        return false;
    }
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    if (!out) {
        if (error != nullptr) *error = "write failed: " + path;
        return false;
    }
    if (error != nullptr) error->clear();
    return true;
}

bool load_project_file(const std::string& path, Project& out, std::string* error)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (error != nullptr) *error = "cannot open: " + path;
        return false;
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
    std::string reason;
    if (!deserialize(bytes, out, &reason)) {
        if (error != nullptr) *error = reason;
        return false;
    }
    if (error != nullptr) error->clear();
    return true;
}
} // namespace xel::anim
