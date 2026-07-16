#include "content_browser_view.h"

#include "icons_tabler.h"
#include <editor.h>
#include <editor_application.h>

namespace codex::editor {
    namespace stdfs = std::filesystem;

    static const char* icon_for_asset(const AssetMetadata& meta)
    {
        if (meta.type == "Texture2D")
            return ICON_TI_PHOTO;
        if (meta.type == "CXXSource")
            return ICON_TI_BRAND_CPP;
        if (meta.type == "CXXHeader")
            return ICON_TI_LETTER_H;
        if (meta.type == "Shader")
            return ICON_TI_SPHERE;
        if (meta.type == "Scene")
            return ICON_TI_SPHERE;
        if (meta.type == "Prefab")
            return ICON_TI_PACKAGE;
        return ICON_TI_FILE;
    }

    static std::string truncate_name(const std::string& name, f32 max_width)
    {
        if (ImGui::CalcTextSize(name.c_str()).x <= max_width)
            return name;
        std::string s = name;
        while (!s.empty() && ImGui::CalcTextSize((s + "...").c_str()).x > max_width)
            s.pop_back();
        return s + "...";
    }

    void ContentBrowserView::on_init()
    { log(Info, "Waiting for asset registry to complete scan..."); }

    void ContentBrowserView::on_imgui_render()
    {
        // TODO: Change to std::lock_guard when we modify assets from here!
        // For now it's just read only so std::shared_lock is fine.

        ImGui::Begin("Content Browser");

        if (ImGui::BeginTable("##cb_layout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("##tree", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableSetupColumn("##grid", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::BeginChild("##cb_tree");

            rebuild_tree();
            TreeNode root_node_cpy;
            {
                std::shared_lock guard{ mutex_ };
                root_node_cpy = root_node_;
            }
            render_directory_tree(root_node_cpy, "/");

            ImGui::EndChild();

            ImGui::TableNextColumn();
            ImGui::BeginChild("##cb_grid");

            render_breadcrumb();
            ImGui::BeginChild("##cb_grid_scroll", { 0, -ImGui::GetFrameHeightWithSpacing() });
            render_asset_grid();
            ImGui::EndChild();
            render_icon_size_slider();

            ImGui::EndChild();

            ImGui::EndTable();
        }

        ImGui::End();
    }

    void ContentBrowserView::on_update([[maybe_unused]] const f32 dt)
    {
        auto d = get_descriptor().lock();

        cxensure(d, "descriptor should not be null while updating content browser");

        if (d->registry_state == AssetRegistryState::Succeeded) {
            static bool content_init_done = false;

            if (!content_init_done) {
                root_path_ = AssetManager::root_dir();
                chdir(root_path_);
                refresh(current_path_);
                log(Info, "Cache refreshed");
                content_init_done = true;
            } else if (dirty_ || last_registry_revision_ != d->asset_registry_revision.load()) {
                std::scoped_lock guard{ mutex_ };
                refresh_nolock(current_path_);
                last_registry_revision_ = d->asset_registry_revision.load();
            }
        }
    }

    void ContentBrowserView::chdir_nolock(const std::string& path)
    {
        current_path_ = (path.back() == '/') ? path.substr(0, path.size() - 1) : path;
        dirty_        = true;
    }

    void ContentBrowserView::chdir(const std::string& path)
    {
        std::scoped_lock guard{ mutex_ };
        chdir_nolock(path);
    }

    void ContentBrowserView::refresh_nolock([[maybe_unused]] const std::string& path)
    {
        auto desc_ref = get_descriptor();
        cxensure(!desc_ref.expired(), "descriptor should not have expired while refreshing content browser");
        auto desc = desc_ref.lock();

        if (desc->registry_state == AssetRegistryState::Succeeded) {
            cache_.clear();
            AssetManager::registry().for_each(
                [this](const AssetMetadata& meta)
                {
                    const stdfs::path path = stdfs::path{ root_path_ } / meta.path.path();
                    if (path.parent_path().generic_string() == current_path_)
                        cache_.push_back(meta);
                    log(Verbose, "path.parent_path(): {} == current_path: {}", path.parent_path().generic_string(),
                        current_path_);
                });
            dirty_ = false;
        }
    }

    void ContentBrowserView::refresh(const std::string& path)
    {
        std::scoped_lock guard{ mutex_ };
        refresh_nolock(path);
    }

    std::string_view ContentBrowserView::cwd() const
    {
        std::shared_lock guard{ mutex_ };
        return current_path_;
    }

    void ContentBrowserView::rebuild_tree()
    {
        std::scoped_lock guard{ mutex_ };
        auto             d = get_descriptor().lock();

        cxensure(d, "descriptor should not be null while rebuilding content browser tree");

        root_node_.name.clear();
        root_node_.children.clear();

        std::vector<std::string> dirlist =
            EditorApplication::vfs().list(root_path_, fs::ListOptions::DirsOnly | fs::ListOptions::Recursive);
        std::sort(dirlist.begin(), dirlist.end());

        for (const std::string& e : dirlist) {
            TreeNode*                      cur_node = &root_node_;
            const std::vector<std::string> xsplit   = util::str::split(e, '/');
            if (root_node_.name.empty())
                root_node_.name = xsplit.front();

            for (const std::string& c : xsplit) {
                if (c == cur_node->name)
                    continue;

                if (auto it = cur_node->children.find(c); it != cur_node->children.end()) {
                    cur_node = &it->second;
                } else {
                    auto node = TreeNode{
                        .name     = c,
                        .children = {},
                    };

                    cur_node->children[c] = std::move(node);

                    cur_node = &cur_node->children[c];
                }
            }
        }
    }

    void ContentBrowserView::render_directory_tree(const TreeNode& node, std::string path)
    {
        const std::string full_path = path + node.name + "/";
        const std::string str_id    = fmt::format("{}##{}", node.name, path);

        ImGuiID     id      = ImGui::GetID(str_id.c_str());
        bool        is_open = ImGui::GetStateStorage()->GetBool(id, false);
        const char* icon    = is_open ? ICON_TI_FOLDER_OPEN : ICON_TI_FOLDER;

        bool open = ImGui::TreeNodeEx(str_id.c_str(), (current_path_ == full_path) ? ImGuiTreeNodeFlags_Selected : 0,
                                      "%s  %s", icon, node.name.c_str());

        if (ImGui::IsItemClicked())
            chdir(full_path);

        if (open) {
            for (const auto& [name, children] : node.children)
                render_directory_tree(children, full_path);
            ImGui::TreePop();
        }
    }

    void ContentBrowserView::render_asset_grid()
    {
        auto d = get_descriptor().lock();

        assert(d);

        const ImGuiStyle& style  = ImGui::GetStyle();
        const f32         text_h = ImGui::GetTextLineHeight();
        const f32         item_h = icon_size_ + text_h + style.ItemSpacing.y * 2.0f;
        const i32 columns = std::max(1, (i32)(ImGui::GetContentRegionAvail().x / (icon_size_ + style.ItemSpacing.x)));

        ImGui::Columns(columns, nullptr, false);

        for (AssetMetadata& meta : cache_) {
            const bool        selected     = meta.path.uuid() == selected_;
            const ImVec2      screen_pos   = ImGui::GetCursorScreenPos();
            const std::string btn_id       = fmt::format("##{}", meta.path.uuid());
            std::string       display_name = stdfs::path{ meta.path.path() }.filename().generic_string();

            const bool clicked = ImGui::InvisibleButton(btn_id.c_str(), { icon_size_, item_h });
            const bool hovered = ImGui::IsItemHovered();
            if (clicked) {
                selected_         = meta.path.uuid();
                d->selected_asset = selected_;
            }

            if (selected || hovered) {
                ImU32 color =
                    selected ? ImGui::GetColorU32(ImGuiCol_Header) : ImGui::GetColorU32(ImGuiCol_HeaderHovered);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    screen_pos, { screen_pos.x + icon_size_, screen_pos.y + item_h }, color, 4.0f);
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                UUID uuid = meta.path.uuid();
                ImGui::SetDragDropPayload("CX_ASSET", &uuid, sizeof(uuid));
                ImGui::Text("%s  %s", icon_for_asset(meta), display_name.c_str());
                ImGui::EndDragDropSource();
            }

            // Icon centered in the icon area (XL font rasterized at 64px)
            render_asset_based_on_type(meta, screen_pos);

            // Name truncated and centered below icon
            const std::string display_name_trunc = truncate_name(display_name, icon_size_);
            const f32         name_w             = ImGui::CalcTextSize(display_name_trunc.c_str()).x;
            ImGui::SetCursorScreenPos(
                { screen_pos.x + (icon_size_ - name_w) * 0.5f, screen_pos.y + icon_size_ + style.ItemSpacing.y });
            ImGui::TextUnformatted(display_name_trunc.c_str());

            ImGui::SetCursorScreenPos({ screen_pos.x, screen_pos.y + item_h + style.ItemSpacing.y });
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }

    void ContentBrowserView::render_icon_size_slider()
    {
        const f32 slider_width = 120.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - slider_width);
        ImGui::SetNextItemWidth(slider_width);
        ImGui::SliderFloat("##icon_size", &icon_size_, 48.0f, 128.0f, "");
    }

    void ContentBrowserView::render_breadcrumb()
    {
        std::scoped_lock guard{ mutex_ };

        std::string acc = "/";
        for (const std::string& c : util::str::split(current_path_, '/')) {
            if (c.empty())
                continue;

            acc += c + '/';

            ImGui::SameLine(0, 2);
            ImGui::Text(">");
            ImGui::SameLine(0, 2);

            std::string id = c + "##" + acc;
            if (ImGui::SmallButton(id.c_str()))
                chdir_nolock(acc);
        }
    }

    void ContentBrowserView::render_asset_based_on_type(const AssetMetadata& meta, const ImVec2 screen_pos) noexcept
    {
        if (meta.type == "Texture2D") {
            Shared<gfx::Texture2D> asset = nullptr;
            if (auto it = asset_cache_.find(meta.path.uuid()); it != asset_cache_.end())
                asset = it->second.as<gfx::Texture2D>();
            else {
                asset = AssetManager::load<gfx::Texture2D>(meta.path).as_shared();
                if (asset) {
                    asset_cache_[meta.path.uuid()] = asset.as<void>();
                }
            }

            if (asset) {
                ImGui::SetCursorScreenPos({ screen_pos.x + (0) * 0.5f, screen_pos.y + (0) * 0.5f });
                ImGui::Image(reinterpret_cast<ImTextureID>(asset->gl_id()), { icon_size_, icon_size_ }, { 0, 1 },
                             { 1, 0 });
                return;
            }
        }

        const char* icon = icon_for_asset(meta);

        ImGui::PushFont(Editor::get_xl_icon_font());
        const f32 icon_w  = ImGui::CalcTextSize(icon).x;
        const f32 icon_fh = ImGui::GetTextLineHeight();
        ImGui::SetCursorScreenPos(
            { screen_pos.x + (icon_size_ - icon_w) * 0.5f, screen_pos.y + (icon_size_ - icon_fh) * 0.5f });
        ImGui::Text("%s", icon);
        ImGui::PopFont();
    }
} // namespace codex::editor
