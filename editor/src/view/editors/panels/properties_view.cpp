#include "properties_view.h"

#include "../scene_editor_view.h"
#include "tile_pallete_view.h"

#include <nfd.h>

namespace codex::editor {
    using namespace codex::events;

    void PropertiesView::on_init()
    {
    }

    void PropertiesView::on_imgui_render()
    {
        auto d = this->get_descriptor().lock();

        ImGui::Begin("Entity properties");
        if (d->selected_entity.entity) {
            if (ImGui::Button("Add component"))
                ImGui::OpenPopup("component_popup");
            if (ImGui::BeginPopup("component_popup")) {
                auto entity = d->selected_entity.entity;
                if (!entity.has_component<SpriteRendererComponent>() && ImGui::MenuItem("Sprite Renderer Component")) {
                    entity.add_component<SpriteRendererComponent>(Sprite::empty());
                    ImGui::CloseCurrentPopup();
                } else if (
                    !entity.has_component<NativeBehaviourComponent>() && ImGui::MenuItem("C++ Script Component")) {
                    entity.add_component<NativeBehaviourComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (!entity.has_component<CameraComponent>() && ImGui::MenuItem("Camera Component")) {
                    entity.add_component<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (
                    !entity.has_component<RigidBody2DComponent>() && ImGui::MenuItem("Rigid Body 2D Component")) {
                    entity.add_component<RigidBody2DComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (
                    !entity.has_component<BoxCollider2DComponent>() && ImGui::MenuItem("Box Collider 2D Component")) {
                    entity.add_component<BoxCollider2DComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (!entity.has_component<CircleCollider2DComponent>() &&
                           ImGui::MenuItem("Circle Collider 2D Component")) {
                    entity.add_component<CircleCollider2DComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (
                    !entity.has_component<GridRendererComponent>() && ImGui::MenuItem("Grid Renderer Component")) {
                    entity.add_component<GridRendererComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (!entity.has_component<TilemapComponent>() && ImGui::MenuItem("Tilemap Component")) {
                    entity.add_component<TilemapComponent>();
                    if (!entity.has_component<GridRendererComponent>())
                        entity.add_component<GridRendererComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (!entity.has_component<TilesetAnimationComponent>() &&
                           ImGui::MenuItem("Tileset Animation Component")) {
                    entity.add_component<TilesetAnimationComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (!entity.has_component<AudioSourceComponent>() && ImGui::MenuItem("Audio Source Component")) {
                    entity.add_component<AudioSourceComponent>();
                    ImGui::CloseCurrentPopup();
                } else if (
                    !entity.has_component<AudioListenerComponent>() && ImGui::MenuItem("Audio Listener Component")) {
                    entity.add_component<AudioListenerComponent>();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (d->selected_entity.entity.has_component<TransformComponent>()) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Transform Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selected_entity.entity.get_component<TransformComponent>();
                    SceneEditorView::draw_vec3_control("Position: ", c.position, d->column_width);
                    SceneEditorView::draw_vec3_control("Rotation: ", c.rotation, d->column_width);
                    SceneEditorView::draw_vec3_control("Scale: ", c.scale, d->column_width, 0.01f);
                }
            }
            if (d->selected_entity.entity.has_component<NativeBehaviourComponent>()) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("C++ Script Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selected_entity.entity.get_component<NativeBehaviourComponent>();

                    // Attached scripts.
                    const bool compiling = (d->compilation_state.load() == CompilationState::Compiling);

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Attached behaviours: ");
                    ImGui::NextColumn();

                    if (compiling)
                        ImGui::BeginDisabled();
                    if (ImGui::BeginCombo("###script_combo", "Select a script")) {
                        auto vec = NBMan::registered_types();
                        for (const auto& script : vec) {
                            const auto& behaviours = c.behaviours();

                            const auto it = std::find_if(behaviours.cbegin(), behaviours.cend(),
                                                         [&script](const auto& e) { return e.first == script; });

                            if (it == behaviours.end()) {
                                if (ImGui::Selectable(script.c_str(), false)) {
                                    auto script_instance = NBMan::create_instance(script);
                                    if (script_instance) {
                                        c.attach(std::move(script_instance));
                                    } else {
                                        error("Failed to create an NB instance of: {}", script);
                                    }
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                    if (compiling)
                        ImGui::EndDisabled();

                    ImGui::Columns(1);

                    // Display attached scripts and their serialized fields.
                    auto&                  behaviours = c.behaviours();
                    std::list<std::string> possible_scripts_to_detach;
                    for (auto it = behaviours.begin(); it != behaviours.end(); ++it) {
                        auto& [k, v] = *it;

                        const auto& type_info = v->type_info();
                        const auto  type_name = std::string{ type_info.type_name() };

                        if (ImGui::CollapsingHeader(type_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                            if (ImGui::Button("Detach script")) {
                                possible_scripts_to_detach.push_back(it->first);
                                continue;
                            }

                            const auto properties = type_info.properties();
                            for (const auto& prop : properties) {
                                const auto prop_id = fmt::format("##nb-{}", prop.name);

                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->column_width);
                                ImGui::Text("%s", prop.name.c_str());
                                ImGui::NextColumn();

                                switch (prop.type) {
                                    using enum rf::PropertyType;

                                    case I32:
                                    case U32: {
                                        ImGui::DragInt(prop_id.c_str(),
                                                       type_info.property_value<i32>(v.get(), prop.name));
                                        break;
                                    }
                                    case F32:
                                    case F64:
                                    case F128: {
                                        ImGui::DragFloat(prop_id.c_str(),
                                                         type_info.property_value<f32>(v.get(), prop.name));
                                        break;
                                    }
                                    case String: {
                                        ImGui::InputText(prop_id.c_str(),
                                                         type_info.property_value<std::string>(v.get(), prop.name));
                                        break;
                                    }
                                    case Boolean: {
                                        ImGui::Checkbox(prop_id.c_str(),
                                                        type_info.property_value<bool>(v.get(), prop.name));
                                        break;
                                    }
                                    case Vector2f: {
                                        SceneEditorView::draw_vec2_control(
                                            prop_id.c_str(),
                                            *type_info.property_value<math::Vector2f>(v.get(), prop.name),
                                            d->column_width);
                                        break;
                                    }
                                    case Vector3f: {
                                        SceneEditorView::draw_vec3_control(
                                            prop_id.c_str(),
                                            *type_info.property_value<math::Vector3f>(v.get(), prop.name),
                                            d->column_width);
                                        break;
                                    }
                                    default: break; // throw CodexException("Should not happen."); break;
                                }
                                ImGui::Columns(1);
                            }
                        }
                    }
                    for (const auto& e : possible_scripts_to_detach)
                        c.detach(e);
                }
                if (remove)
                    d->selected_entity.entity.remove_component<NativeBehaviourComponent>();
            }
            if (d->selected_entity.entity.has_component<SpriteRendererComponent>()) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Sprite Renderer Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c       = d->selected_entity.entity.get_component<SpriteRendererComponent>();
                    auto& sprite  = c.sprite();
                    auto  texture = sprite.texture();

                    // Texture prewview image
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Texture: ");
                        ImGui::NextColumn();

                        ImGui::BeginGroup();
                        if (sprite)
                            ImGui::Image(static_cast<ImTextureID>(texture->gl_id()), { 100.0f, 100.0f }, { 0, 1 },
                                         { 1, 0 });
                        else
                            ImGui::Text("No bound texture.");

                        static char tex_path_buf[256] = {};
                        ImGui::SetNextItemWidth(100.0f);
                        ImGui::InputText("##tex_path", tex_path_buf, sizeof(tex_path_buf));
                        ImGui::SameLine();
                        if (ImGui::Button("Load")) {
                            // TODO: Check if AssetManager::load<T> returned a valid object, or maybe make load throw an
                            // exception?
                            sprite.set_texture(AssetManager::load<gfx::Texture2D>(tex_path_buf));
                        }
                        ImGui::EndGroup();
                        ImGui::Columns(1);
                        ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    }

                    // Texture coordinates
                    const auto tex_coords = sprite.texture_coords();
                    Vector2f   pos{ tex_coords.x, tex_coords.y };
                    Vector2f   size{ tex_coords.w, tex_coords.h };
                    SceneEditorView::draw_vec2_control("Texture position: ", pos, d->column_width);
                    SceneEditorView::draw_vec2_control("Texture size: ", size, d->column_width);
                    sprite.set_texture_coords({ pos.x, pos.y, size.x, size.y });

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto sprite_size = sprite.size();
                    SceneEditorView::draw_vec2_control("Sprite size: ", sprite_size, d->column_width);
                    sprite.set_size(sprite_size);

                    // Texture colour picker
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Sprite overlay: ");
                    ImGui::NextColumn();

                    auto                colour = sprite.colour();
                    ImGuiColorEditFlags flags  = 0;
                    flags |= ImGuiColorEditFlags_AlphaBar;
                    flags |= ImGuiColorEditFlags_DisplayRGB; // Override display
                                                             // mode
                    f32 temp_colour[4]{ colour.x, colour.y, colour.z, colour.w };
                    ImGui::ColorPicker4("##color_picker_src", temp_colour, flags);
                    colour = { temp_colour[0], temp_colour[1], temp_colour[2], temp_colour[3] };
                    sprite.set_colour(Vector4f{ temp_colour[0], temp_colour[1], temp_colour[2], temp_colour[3] });
                    ImGui::Columns(1);

                    static i32 z_index;
                    z_index = sprite.z_index();
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Z Index: ");
                    ImGui::NextColumn();
                    ImGui::DragInt("##drag_int", &z_index);
                    ImGui::Columns(1);
                    sprite.set_z_index(z_index);
                }
                if (remove)
                    d->selected_entity.entity.remove_component<SpriteRendererComponent>();
            }
            if (d->selected_entity.entity.has_component<CameraComponent>()) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selected_entity.entity.get_component<CameraComponent>();

                    i32 width     = c.camera.width();
                    i32 height    = c.camera.height();
                    f32 near_clip = c.camera.near_clip();
                    f32 far_clip  = c.camera.far_clip();
                    f32 fov       = c.camera.fov();
                    f32 pan       = c.camera.pan();

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Primary camera");
                    ImGui::NextColumn();
                    ImGui::Checkbox("###primary_checkbox", &c.primary);
                    ImGui::Columns(1);

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Width");
                    ImGui::NextColumn();
                    ImGui::DragInt("###wwcamidth", &width);
                    ImGui::Columns(1);

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Height");
                    ImGui::NextColumn();
                    ImGui::DragInt("###wcamheight", &height);
                    ImGui::Columns(1);

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Pan");
                    ImGui::NextColumn();
                    ImGui::DragFloat("###wcampan", &pan);
                    ImGui::Columns(1);

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    if (c.camera.projection_type() == scene::Camera::ProjectionType::Perspective) {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("FOV");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###fov", &fov);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Near clip");
                    ImGui::NextColumn();
                    ImGui::DragFloat("###near_clip", &near_clip);
                    ImGui::Columns(1);
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    ImGui::Text("Far clip");
                    ImGui::NextColumn();
                    ImGui::DragFloat("###far_clip", &far_clip);
                    ImGui::Columns(1);

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->column_width);
                    std::string title = (c.camera.projection_type() == scene::Camera::ProjectionType::Orthographic)
                                            ? "Orthographic"
                                            : "Perspective";
                    ImGui::Text("Projection type");
                    ImGui::NextColumn();
                    if (ImGui::BeginCombo("###begin_combo", title.c_str())) {
                        if (ImGui::Selectable("Orthographic", false))
                            c.camera.set_projection_type(scene::Camera::ProjectionType::Orthographic);
                        if (ImGui::Selectable("Perspective", false))
                            c.camera.set_projection_type(scene::Camera::ProjectionType::Perspective);
                        ImGui::EndCombo();
                    }

                    ImGui::Columns(1);

                    // Update the values if modified.
                    if (width != c.camera.width())
                        c.camera.set_width(width);
                    if (height != c.camera.height())
                        c.camera.set_height(height);
                    if (fov != c.camera.fov())
                        c.camera.set_fov(fov);
                    if (near_clip != c.camera.near_clip())
                        c.camera.set_near_clip(near_clip);
                    if (far_clip != c.camera.far_clip())
                        c.camera.set_far_clip(far_clip);
                    if (pan != c.camera.pan())
                        c.camera.set_pan(pan);
                }
                if (remove)
                    d->selected_entity.entity.remove_component<CameraComponent>();
            }
            if (d->selected_entity.entity.has_component<RigidBody2DComponent>()) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Rigid Body 2D Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selected_entity.entity.get_component<RigidBody2DComponent>();

                    // Body type.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Body type");
                        ImGui::NextColumn();

                        using BodyType = RigidBody2DComponent::BodyType;

                        std::string title;
                        switch (c.body_type) {
                            using enum BodyType;

                            case Static: title = "Static"; break;
                            case Dynamic: title = "Dynamic"; break;
                            case Kinematic: title = "Kinematic"; break;
                        }

                        if (ImGui::BeginCombo("###body_combo", title.c_str())) {
                            if (ImGui::Selectable("Static", c.body_type == BodyType::Static))
                                c.body_type = BodyType::Static;
                            if (ImGui::Selectable("Dynamic", c.body_type == BodyType::Dynamic))
                                c.body_type = BodyType::Dynamic;
                            if (ImGui::Selectable("Kinematic", c.body_type == BodyType::Kinematic))
                                c.body_type = BodyType::Kinematic;
                            ImGui::EndCombo();
                        }
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Fixed rotation.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Fixed rotation");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###fixed_box", &c.fixed_rotation);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Linear damping.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Linear damping");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###linear_drag", &c.linear_damping);
                        ImGui::Columns(1);
                    }

                    // Angular damping.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Angular damping");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###angular_drag", &c.angular_damping);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Enabled.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Simulate");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###enabled", &c.enabled);
                        ImGui::Columns(1);
                    }

                    // Gravity scale.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Gravity scale");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###gravity_drag", &c.gravity_scale);
                        ImGui::Columns(1);
                    }
                }
                if (remove)
                    d->selected_entity.entity.remove_component<RigidBody2DComponent>();
            }
            if (d->selected_entity.entity.has_component<BoxCollider2DComponent>()) {
                auto& c = d->selected_entity.entity.get_component<BoxCollider2DComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Box Collider 2D Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    // Offset
                    {
                        SceneEditorView::draw_vec2_control("Offset", c.offset, d->column_width);
                    }

                    // Size
                    {
                        SceneEditorView::draw_vec2_control("Size", c.size, d->column_width);
                    }

                    // Physics material 2d.
                    {
                        draw_physics_material_2d_control(c.physics_material, d->column_width);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                }
                if (remove)
                    d->selected_entity.entity.remove_component<BoxCollider2DComponent>();
            }
            if (d->selected_entity.entity.has_component<CircleCollider2DComponent>()) {
                auto& c = d->selected_entity.entity.get_component<CircleCollider2DComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Circle Collider 2D Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    // Offset.
                    {
                        SceneEditorView::draw_vec2_control("Offset", c.offset, d->column_width);
                    }

                    // Radius.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Radius");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###radius_drag", &c.radius);
                        ImGui::Columns(1);
                    }

                    // Physics material 2d.
                    {
                        draw_physics_material_2d_control(c.physics_material, d->column_width);
                    }
                }
                if (remove)
                    d->selected_entity.entity.remove_component<CircleCollider2DComponent>();
            }
            if (d->selected_entity.entity.has_component<GridRendererComponent>()) {
                auto& c = d->selected_entity.entity.get_component<GridRendererComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Grid Renderer Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    // Grid cell size
                    {
                        SceneEditorView::draw_vec2_control("Grid Cell Size", c.cell_size, d->column_width);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Gird colour
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Grid colour");
                        ImGui::NextColumn();

                        ImGuiColorEditFlags flags = 0;
                        flags |= ImGuiColorEditFlags_AlphaBar;
                        flags |= ImGuiColorEditFlags_DisplayRGB; // Override display
                                                                 // mode
                        f32 temp_colour[4]{ c.colour.x, c.colour.y, c.colour.z, c.colour.w };
                        ImGui::ColorPicker4("##grid_colour_picker", temp_colour, flags);
                        c.colour = Vector4f{ temp_colour[0], temp_colour[1], temp_colour[2], temp_colour[3] };
                        ImGui::Columns(1);
                    }
                }
                if (remove)
                    d->selected_entity.entity.remove_component<GridRendererComponent>();
            }
            if (d->selected_entity.entity.has_component<TilemapComponent>()) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Tilemap Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selected_entity.entity.get_component<TilemapComponent>();

                    // Sprite preview
                    {
                        // Texture prewview image
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->column_width);
                            ImGui::Text("Texture: ");
                            ImGui::NextColumn();

                            ImGui::BeginGroup();
                            if (c.sprite)
                                ImGui::Image(static_cast<ImTextureID>(c.sprite.texture()->gl_id()),
                                             { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
                            else
                                ImGui::Text("No bound texture.");

                            static char s_tex_path_buf[256] = {};
                            ImGui::SetNextItemWidth(120.0f);
                            ImGui::InputText("##tex_path", s_tex_path_buf, sizeof(s_tex_path_buf));
                            ImGui::SameLine();
                            if (ImGui::Button("Load")) {
                                c.sprite.set_texture(AssetManager::load<gfx::Texture2D>(s_tex_path_buf));
                            }
                            ImGui::EndGroup();
                            ImGui::Columns(1);
                            ImGui::Dummy(ImVec2(0.0f, 10.0f));
                        }

                        // Texture filter mode
                        if (c.sprite) {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->column_width);
                            ImGui::Text("Filter mode");
                            ImGui::NextColumn();

                            auto        texture      = c.sprite.texture();
                            const char* preview_item = nullptr;
                            const auto& props        = texture->properties();
                            switch (props.filter_mode) {
                                case opengl::TextureFilterMode::Linear: preview_item = "Linear"; break;
                                case opengl::TextureFilterMode::Nearest: preview_item = "Nearest"; break;
                            }
                            if (ImGui::BeginCombo("##texture_filter_mode", preview_item)) {
                                if (ImGui::Selectable("Nearest",
                                                      props.filter_mode == opengl::TextureFilterMode::Nearest)) {
                                    if (props.filter_mode != opengl::TextureFilterMode::Nearest) {
                                        auto new_props        = props;
                                        new_props.filter_mode = opengl::TextureFilterMode::Nearest;

                                        // TODO: Proper re-import ?
                                        auto path = texture.path();
                                        AssetManager::reimport<gfx::Texture2D>(
                                            path.uuid(), gfx::Texture2D::ImportSettings{ new_props });
                                    }
                                }
                                if (ImGui::Selectable("Linear",
                                                      props.filter_mode == opengl::TextureFilterMode::Linear)) {
                                    if (props.filter_mode != opengl::TextureFilterMode::Linear) {
                                        auto new_props        = props;
                                        new_props.filter_mode = opengl::TextureFilterMode::Linear;

                                        // TODO: Proper re-import ?
                                        auto path = texture.path();
                                        AssetManager::reimport<gfx::Texture2D>(
                                            path.uuid(), gfx::Texture2D::ImportSettings{ new_props });
                                    }
                                }
                                ImGui::EndCombo();
                            }

                            ImGui::Columns(1);
                        }
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Layer
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Layer");
                        ImGui::NextColumn();
                        ImGui::DragInt("###layer_dragger", &c.current_layer);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Gird size
                    {
                        auto new_grid_size = c.grid_size;
                        SceneEditorView::draw_vec2_control("Grid size", new_grid_size, d->column_width);
                        if (new_grid_size != c.grid_size) {
                            c.grid_size                                                                = new_grid_size;
                            d->selected_entity.entity.get_component<GridRendererComponent>().cell_size = new_grid_size;
                        }
                    }

                    // Tile size
                    {
                        SceneEditorView::draw_vec2_control("Tile size", c.tile_size, d->column_width);
                    }

                    if (ImGui::Button("Open tile pallete")) {
                        auto& panel = this->attach_panel<TilePalleteView>();
                        panel.set_entity(d->selected_entity.entity);
                        panel.focus();
                    }
                }
                if (remove)
                    d->selected_entity.entity.remove_component<TilemapComponent>();
            }
            if (d->selected_entity.entity.has_component<TilesetAnimationComponent>()) {
                auto& c      = d->selected_entity.entity.get_component<TilesetAnimationComponent>();
                bool  remove = false;
                bool  open   = ImGui::CollapsingHeader("Tileset Animation Componnet", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    // Sprite preview
                    {
                        // Texture prewview image
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->column_width);
                            ImGui::Text("Texture: ");
                            ImGui::NextColumn();

                            ImGui::BeginGroup();
                            if (c.sprite)
                                ImGui::Image(static_cast<ImTextureID>(c.sprite.texture()->gl_id()),
                                             { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
                            else
                                ImGui::Text("No bound texture.");

                            static char s_tex_path_buf[256] = {};
                            ImGui::SetNextItemWidth(120.0f);
                            ImGui::InputText("##tex_path", s_tex_path_buf, sizeof(s_tex_path_buf));
                            ImGui::SameLine();
                            if (ImGui::Button("Load")) {
                                c.sprite.set_texture(AssetManager::load<gfx::Texture2D>(s_tex_path_buf));
                            }
                            ImGui::EndGroup();
                            ImGui::Columns(1);
                            ImGui::Dummy(ImVec2(0.0f, 10.0f));
                        }

                        // Texture filter mode
                        if (c.sprite) {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->column_width);
                            ImGui::Text("Filter mode");
                            ImGui::NextColumn();

                            auto        texture      = c.sprite.texture();
                            const char* preview_item = nullptr;
                            const auto& props        = texture->properties();
                            switch (props.filter_mode) {
                                case opengl::TextureFilterMode::Linear: preview_item = "Linear"; break;
                                case opengl::TextureFilterMode::Nearest: preview_item = "Nearest"; break;
                            }
                            if (ImGui::BeginCombo("##texture_filter_mode", preview_item)) {
                                if (ImGui::Selectable("Nearest",
                                                      props.filter_mode == opengl::TextureFilterMode::Nearest)) {
                                    if (props.filter_mode != opengl::TextureFilterMode::Nearest) {
                                        auto new_props        = props;
                                        new_props.filter_mode = opengl::TextureFilterMode::Nearest;

                                        // TODO: Proper re-import ?
                                        auto path = texture.path();
                                        AssetManager::reimport<gfx::Texture2D>(
                                            path.uuid(), gfx::Texture2D::ImportSettings{ new_props });
                                    }
                                }
                                if (ImGui::Selectable("Linear",
                                                      props.filter_mode == opengl::TextureFilterMode::Linear)) {
                                    if (props.filter_mode != opengl::TextureFilterMode::Linear) {
                                        auto new_props        = props;
                                        new_props.filter_mode = opengl::TextureFilterMode::Linear;

                                        // TODO: Proper re-import ?
                                        auto path = texture.path();
                                        AssetManager::reimport<gfx::Texture2D>(
                                            path.uuid(), gfx::Texture2D::ImportSettings{ new_props });
                                    }
                                }
                                ImGui::EndCombo();
                            }

                            ImGui::Columns(1);
                        }

                        // Add animation button
                        {
                            if (ImGui::Button("Add an animation")) {
                                c.animations["animation #" + std::to_string(c.animations.size())] =
                                    TilesetAnimationComponent::Animation{};
                            }
                        }
                    }

                    for (auto& [name, anim] : c.animations) {
                        if (ImGui::TreeNodeEx((name + "##tile_set_anim_" + name).c_str())) {
                            // Starting tile
                            {
                                SceneEditorView::draw_vec2_control("Starting tile", anim.starting_tile,
                                                                   d->column_width);
                            }

                            // Frame count
                            {
                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->column_width);
                                ImGui::Text("Frame count");
                                ImGui::NextColumn();
                                ImGui::DragInt(("###drag_int_" + name).c_str(),
                                               reinterpret_cast<i32*>(&anim.frame_count), 1.0f, 0);
                                ImGui::Columns(1);
                            }

                            // Frame rate
                            {
                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->column_width);
                                ImGui::Text("Frame rate");
                                ImGui::NextColumn();
                                ImGui::DragFloat(("###drag_f32_" + name).c_str(), &anim.frame_rate, 1.0f, 0);
                                ImGui::Columns(1);
                            }

                            // Animation preview
                            {
                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->column_width);
                                ImGui::Text("Texture: ");
                                ImGui::NextColumn();

                                ImGui::BeginGroup();
                                if (c.sprite)
                                    ImGui::Image(static_cast<ImTextureID>(c.sprite.texture()->gl_id()),
                                                 { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
                                else
                                    ImGui::Text("No bound texture.");

                                ImGui::EndGroup();
                                ImGui::Columns(1);
                                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                            }

                            ImGui::TreePop();
                        }
                    }
                }
                if (remove)
                    d->selected_entity.entity.remove_component<TilesetAnimationComponent>();
            }
            if (d->selected_entity.entity.has_component<AudioSourceComponent>()) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Audio Source Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selected_entity.entity.get_component<AudioSourceComponent>();

                    // Event path (searchable dropdown)
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Event path");
                        ImGui::NextColumn();

                        static std::string              search_filter;
                        static std::vector<std::string> cached_events;
                        static bool                     needs_refresh = true;

                        const char* preview = c.event_path.empty() ? "Select an event..." : c.event_path.c_str();
                        if (ImGui::BeginCombo("###audio_event_path", preview)) {
                            if (needs_refresh) {
                                cached_events = ax::AudioManager::get_all_event_paths();
                                needs_refresh = false;
                            }

                            ImGui::InputTextWithHint("###event_search", "Search...", &search_filter);
                            ImGui::Separator();

                            for (const auto& ev : cached_events) {
                                if (!search_filter.empty() && ev.find(search_filter) == std::string::npos)
                                    continue;

                                const bool is_selected = (ev == c.event_path);
                                if (ImGui::Selectable(ev.c_str(), is_selected)) {
                                    c.event_path = ev;
                                    search_filter.clear();
                                }
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }

                            ImGui::EndCombo();
                        } else {
                            needs_refresh = true;
                            search_filter.clear();
                        }

                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Sound path
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Sound file");
                        ImGui::NextColumn();

                        static std::string sound_path_str;
                        sound_path_str = c.sound_path.string();
                        ImGui::InputText("###audio_sound_path", &sound_path_str);
                        c.sound_path = sound_path_str;

                        if (ImGui::Button("Browse##audio_browse", { 100, 0 })) {
                            nfdu8char_t*      outPath   = nullptr;
                            nfdu8filteritem_t filters[] = { { "Audio Files", "wav,ogg,mp3,flac" } };
                            if (NFD_OpenDialogU8(&outPath, filters, 1, nullptr) == NFD_OKAY) {
                                c.sound_path = std::filesystem::relative(outPath, std::filesystem::current_path());
                                NFD_FreePathU8(outPath);
                            }
                        }
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Volume
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Volume");
                        ImGui::NextColumn();
                        ImGui::SliderFloat("###audio_volume", &c.volume, 0.0f, 2.0f);
                        ImGui::Columns(1);
                    }

                    // Pitch
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Pitch");
                        ImGui::NextColumn();
                        ImGui::SliderFloat("###audio_pitch", &c.pitch, 0.1f, 4.0f);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Loop
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Loop");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###audio_loop", &c.loop);
                        ImGui::Columns(1);
                    }

                    // Play on start
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("Play on start");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###audio_play_on_start", &c.play_on_start);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // 3D Audio
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->column_width);
                        ImGui::Text("3D Audio");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###audio_is_3d", &c.is_3d);
                        ImGui::Columns(1);
                    }

                    if (c.is_3d) {
                        // Min distance
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->column_width);
                            ImGui::Text("Min distance");
                            ImGui::NextColumn();
                            ImGui::DragFloat("###audio_min_dist", &c.min_distance, 0.1f, 0.0f, c.max_distance);
                            ImGui::Columns(1);
                        }

                        // Max distance
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->column_width);
                            ImGui::Text("Max distance");
                            ImGui::NextColumn();
                            ImGui::DragFloat("###audio_max_dist", &c.max_distance, 1.0f, c.min_distance, 10000.0f);
                            ImGui::Columns(1);
                        }
                    }

                    // Event parameters
                    if (!c.event_path.empty()) {
                        ImGui::Dummy(ImVec2(0.0f, 10.0f));

                        static std::vector<ax::EventParameterInfo> cached_params;
                        static std::string                         cached_params_event;

                        // Refresh when the event path changes.
                        if (cached_params_event != c.event_path) {
                            cached_params       = ax::AudioManager::get_event_parameters(c.event_path);
                            cached_params_event = c.event_path;

                            // Populate defaults for parameters not yet in the map.
                            for (const auto& p : cached_params) {
                                if (!c.parameters.contains(p.name))
                                    c.parameters[p.name] = p.defaultValue;
                            }
                        }

                        if (!cached_params.empty() && ImGui::TreeNodeEx("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
                            for (const auto& p : cached_params) {
                                auto& val = c.parameters[p.name];
                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->column_width);
                                ImGui::Text("%s", p.name.c_str());
                                ImGui::NextColumn();
                                ImGui::SliderFloat(("###param_" + p.name).c_str(), &val, p.minimum, p.maximum);
                                ImGui::Columns(1);
                            }
                            ImGui::TreePop();
                        }
                    }
                }
                if (remove)
                    d->selected_entity.entity.remove_component<AudioSourceComponent>();
            }
            if (d->selected_entity.entity.has_component<AudioListenerComponent>()) {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open   = ImGui::CollapsingHeader("Audio Listener Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open) {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Text("Position and orientation are taken from the Transform Component.");
                }
                if (remove)
                    d->selected_entity.entity.remove_component<AudioListenerComponent>();
            }
        }

        ImGui::End();
    }

    void PropertiesView::draw_physics_material_2d_control(phys::PhysicsMaterial2D& mat, const f32 columnWidth) noexcept
    {
        if (ImGui::TreeNodeEx("Physics Material 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Density
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("Density");
                ImGui::NextColumn();
                ImGui::DragFloat("###density_drag", &mat.density_);
                ImGui::Columns(1);
            }

            // Friction
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("Friction");
                ImGui::NextColumn();
                ImGui::DragFloat("###friction_drag", &mat.friction_);
                ImGui::Columns(1);
            }

            // Restitution
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("Restitution");
                ImGui::NextColumn();
                ImGui::DragFloat("###restitution_drag", &mat.restitution_);
                ImGui::Columns(1);
            }

            // Restitution threshold
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("Restitution threshold");
                ImGui::NextColumn();
                ImGui::DragFloat("###threshold_drag", &mat.restitution_threshold_);
                ImGui::Columns(1);
            }

            ImGui::TreePop();
        }
    }
} // namespace codex::editor
