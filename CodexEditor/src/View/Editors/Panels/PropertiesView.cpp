#include "PropertiesView.h"

#include "../SceneEditorView.h"
#include "TilePalleteView.h"

#include <nfd.h>

namespace codex::editor {
    using namespace codex::events;

    void PropertiesView::OnInit()
    {
    }

    void PropertiesView::OnImGuiRender()
    {
        auto d = this->GetDescriptor().Lock();

        ImGui::Begin("Entity properties");
        if (d->selectedEntity.entity)
        {
            if (ImGui::Button("Add component"))
                ImGui::OpenPopup("component_popup");
            if (ImGui::BeginPopup("component_popup"))
            {
                auto entity = d->selectedEntity.entity;
                if (!entity.HasComponent<SpriteRendererComponent>() && ImGui::MenuItem("Sprite Renderer Component"))
                {
                    entity.AddComponent<SpriteRendererComponent>(Sprite::Empty());
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<NativeBehaviourComponent>() && ImGui::MenuItem("C++ Script Component"))
                {
                    entity.AddComponent<NativeBehaviourComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<CameraComponent>() && ImGui::MenuItem("Camera Component"))
                {
                    entity.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<RigidBody2DComponent>() && ImGui::MenuItem("Rigid Body 2D Component"))
                {
                    entity.AddComponent<RigidBody2DComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<BoxCollider2DComponent>() && ImGui::MenuItem("Box Collider 2D Component"))
                {
                    entity.AddComponent<BoxCollider2DComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<CircleCollider2DComponent>() &&
                         ImGui::MenuItem("Circle Collider 2D Component"))
                {
                    entity.AddComponent<CircleCollider2DComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<GridRendererComponent>() && ImGui::MenuItem("Grid Renderer Component"))
                {
                    entity.AddComponent<GridRendererComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<TilemapComponent>() && ImGui::MenuItem("Tilemap Component"))
                {
                    entity.AddComponent<TilemapComponent>();
                    if (!entity.HasComponent<GridRendererComponent>())
                        entity.AddComponent<GridRendererComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (
                    !entity.HasComponent<TilesetAnimationComponent>() && ImGui::MenuItem("Tileset Animation Component"))
                {
                    entity.AddComponent<TilesetAnimationComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<AudioSourceComponent>() && ImGui::MenuItem("Audio Source Component"))
                {
                    entity.AddComponent<AudioSourceComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (!entity.HasComponent<AudioListenerComponent>() && ImGui::MenuItem("Audio Listener Component"))
                {
                    entity.AddComponent<AudioListenerComponent>();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (d->selectedEntity.entity.HasComponent<TransformComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Transform Component", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selectedEntity.entity.GetComponent<TransformComponent>();
                    SceneEditorView::DrawVec3Control("Position: ", c.position, d->columnWidth);
                    SceneEditorView::DrawVec3Control("Rotation: ", c.rotation, d->columnWidth);
                    SceneEditorView::DrawVec3Control("Scale: ", c.scale, d->columnWidth, 0.01f);
                }
            }
            if (d->selectedEntity.entity.HasComponent<NativeBehaviourComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("C++ Script Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selectedEntity.entity.GetComponent<NativeBehaviourComponent>();

                    // Attached scripts.
                    const bool compiling = (d->compilationState.load() == CompilationState::Compiling);

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Attached behaviours: ");
                    ImGui::NextColumn();

                    if (compiling)
                        ImGui::BeginDisabled();
                    if (ImGui::BeginCombo("###script_combo", "Select a script"))
                    {
                        auto vec = NBMan::GetRegisteredTypes();
                        for (const auto& script : vec)
                        {
                            const auto& behaviours = c.GetBehaviours();

                            const auto it = std::find_if(behaviours.cbegin(), behaviours.cend(),
                                                         [&script](const auto& e) { return e.first == script; });

                            if (it == behaviours.end())
                            {
                                if (ImGui::Selectable(script.c_str(), false))
                                {
                                    auto script_instance = NBMan::CreateInstance(script);
                                    if (script_instance)
                                    {
                                        c.Attach(std::move(script_instance));
                                    }
                                    else
                                    {
                                        lgx::Get("editor").Log(lgx::Error, "Failed to create an NB instance of: {}",
                                                               script);
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
                    auto&                  behaviours = c.GetBehaviours();
                    std::list<std::string> possible_scripts_to_detach;
                    for (auto it = behaviours.begin(); it != behaviours.end(); ++it)
                    {
                        auto& [k, v] = *it;

                        const auto& type_info = v->GetTypeInfo();
                        const auto  type_name = std::string{ type_info.GetTypeName() };

                        if (ImGui::CollapsingHeader(type_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            if (ImGui::Button("Detach script"))
                            {
                                possible_scripts_to_detach.push_back(it->first);
                                continue;
                            }

                            const auto properties = type_info.GetProperties();
                            for (const auto& prop : properties)
                            {
                                const auto prop_id = fmt::format("##nb-{}", prop.name);

                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->columnWidth);
                                ImGui::Text("%s", prop.name.c_str());
                                ImGui::NextColumn();

                                switch (prop.type)
                                {
                                    using enum rf::PropertyType;

                                    case I32:
                                    case U32: {
                                        ImGui::DragInt(prop_id.c_str(),
                                                       type_info.GetPropertyValue<i32>(v.Get(), prop.name));
                                        break;
                                    }
                                    case F32:
                                    case F64:
                                    case F128: {
                                        ImGui::DragFloat(prop_id.c_str(),
                                                         type_info.GetPropertyValue<f32>(v.Get(), prop.name));
                                        break;
                                    }
                                    case String: {
                                        ImGui::InputText(prop_id.c_str(),
                                                         type_info.GetPropertyValue<std::string>(v.Get(), prop.name));
                                        break;
                                    }
                                    case Boolean: {
                                        ImGui::Checkbox(prop_id.c_str(),
                                                        type_info.GetPropertyValue<bool>(v.Get(), prop.name));
                                        break;
                                    }
                                    case Vector2f: {
                                        SceneEditorView::DrawVec2Control(
                                            prop_id.c_str(),
                                            *type_info.GetPropertyValue<math::Vector2f>(v.Get(), prop.name),
                                            d->columnWidth);
                                        break;
                                    }
                                    case Vector3f: {
                                        SceneEditorView::DrawVec3Control(
                                            prop_id.c_str(),
                                            *type_info.GetPropertyValue<math::Vector3f>(v.Get(), prop.name),
                                            d->columnWidth);
                                        break;
                                    }
                                    default: break; // cx_throw(CodexException, "Should not happen."); break;
                                }
                                ImGui::Columns(1);
                            }
                        }
                    }
                    for (const auto& e : possible_scripts_to_detach)
                        c.Detach(e);
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<NativeBehaviourComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<SpriteRendererComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Sprite Renderer Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c       = d->selectedEntity.entity.GetComponent<SpriteRendererComponent>();
                    auto& sprite  = c.GetSprite();
                    auto  texture = sprite.GetTexture();

                    // Texture prewview image
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Texture: ");
                        ImGui::NextColumn();

                        ImGui::BeginGroup();
                        if (sprite)
                            ImGui::Image(reinterpret_cast<ImTextureID>(texture->GetGlId()), { 100.0f, 100.0f },
                                         { 0, 1 }, { 1, 0 });
                        else
                            ImGui::Text("No bound texture.");

                        if (ImGui::Button("Load texture", { 100, 0 }))
                        {
                            nfdu8char_t*      outPath   = nullptr;
                            nfdu8filteritem_t filters[] = { { "Images", "png,jpg" } };
                            if (NFD_OpenDialogU8(&outPath, filters, 1, nullptr) == NFD_OKAY)
                            {
                                std::filesystem::path relative_path =
                                    std::filesystem::relative(outPath, std::filesystem::current_path());
                                NFD_FreePathU8(outPath);
                                auto res = Resources::Load<gfx::Texture2D>(relative_path);
                                sprite.SetTexture(res);
                            }
                        }
                        ImGui::EndGroup();
                        ImGui::Columns(1);
                        ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    }

                    // Texture coordinates
                    const auto tex_coords = sprite.GetTextureCoords();
                    Vector2f   pos{ tex_coords.x, tex_coords.y };
                    Vector2f   size{ tex_coords.w, tex_coords.h };
                    SceneEditorView::DrawVec2Control("Texture position: ", pos, d->columnWidth);
                    SceneEditorView::DrawVec2Control("Texture size: ", size, d->columnWidth);
                    sprite.SetTextureCoords({ pos.x, pos.y, size.x, size.y });

                    // Texture filter mode
                    if (texture)
                    {
                        ImGui::Dummy(ImVec2(0.0f, 10.0f));
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Texture filter mode: ");
                        ImGui::NextColumn();
                        static int item_current_idx = 0; // Here we store our
                                                         // selection data as an
                                                         // index.
                        const char* preview_item = nullptr;
                        const auto& props        = texture->GetProperties();
                        switch (props.filterMode)
                        {
                            case opengl::TextureFilterMode::Linear: preview_item = "Linear"; break;
                            case opengl::TextureFilterMode::Nearest: preview_item = "Nearest"; break;
                        }
                        if (ImGui::BeginCombo("##texture_filter_mode", preview_item))
                        {
                            if (ImGui::Selectable("Nearest", props.filterMode == opengl::TextureFilterMode::Nearest))
                            {
                                if (props.filterMode != opengl::TextureFilterMode::Nearest)
                                {
                                    auto new_props       = props;
                                    new_props.filterMode = opengl::TextureFilterMode::Nearest;
                                    texture->New(texture->GetFilePath(), new_props);
                                }
                            }
                            if (ImGui::Selectable("Linear", props.filterMode == opengl::TextureFilterMode::Linear))
                            {
                                if (props.filterMode != opengl::TextureFilterMode::Linear)
                                {
                                    auto new_props       = props;
                                    new_props.filterMode = opengl::TextureFilterMode::Linear;
                                    texture->New(texture->GetFilePath(), new_props);
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto sprite_size = sprite.GetSize();
                    SceneEditorView::DrawVec2Control("Sprite size: ", sprite_size, d->columnWidth);
                    sprite.SetSize(sprite_size);

                    // Texture colour picker
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Sprite overlay: ");
                    ImGui::NextColumn();

                    auto                colour = sprite.GetColour();
                    ImGuiColorEditFlags flags  = 0;
                    flags |= ImGuiColorEditFlags_AlphaBar;
                    flags |= ImGuiColorEditFlags_DisplayRGB; // Override display
                                                             // mode
                    f32 temp_colour[4]{ colour.x, colour.y, colour.z, colour.w };
                    ImGui::ColorPicker4("##color_picker_src", temp_colour, flags);
                    colour = { temp_colour[0], temp_colour[1], temp_colour[2], temp_colour[3] };
                    sprite.SetColour(Vector4f{ temp_colour[0], temp_colour[1], temp_colour[2], temp_colour[3] });
                    ImGui::Columns(1);

                    static i32 z_index;
                    z_index = sprite.GetZIndex();
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Z Index: ");
                    ImGui::NextColumn();
                    ImGui::DragInt("##drag_int", &z_index);
                    ImGui::Columns(1);
                    sprite.SetZIndex(z_index);
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<SpriteRendererComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<CameraComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selectedEntity.entity.GetComponent<CameraComponent>();

                    i32 width     = c.camera.GetWidth();
                    i32 height    = c.camera.GetHeight();
                    f32 near_clip = c.camera.GetNearClip();
                    f32 far_clip  = c.camera.GetFarClip();
                    f32 fov       = c.camera.GetFOV();

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Primary camera");
                    ImGui::NextColumn();
                    ImGui::Checkbox("###primary_checkbox", &c.primary);
                    ImGui::Columns(1);

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Width");
                    ImGui::NextColumn();
                    ImGui::DragInt("###width", &width);
                    ImGui::Columns(1);

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Height");
                    ImGui::NextColumn();
                    ImGui::DragInt("###height", &height);
                    ImGui::Columns(1);

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    if (c.camera.GetProjectionType() == scene::Camera::ProjectionType::Perspective)
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("FOV");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###fov", &fov);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Near clip");
                    ImGui::NextColumn();
                    ImGui::DragFloat("###near_clip", &near_clip);
                    ImGui::Columns(1);
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Far clip");
                    ImGui::NextColumn();
                    ImGui::DragFloat("###far_clip", &far_clip);
                    ImGui::Columns(1);

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    std::string title = (c.camera.GetProjectionType() == scene::Camera::ProjectionType::Orthographic)
                                            ? "Orthographic"
                                            : "Perspective";
                    ImGui::Text("Projection type");
                    ImGui::NextColumn();
                    if (ImGui::BeginCombo("###begin_combo", title.c_str()))
                    {
                        if (ImGui::Selectable("Orthographic", false))
                            c.camera.SetProjectionType(scene::Camera::ProjectionType::Orthographic);
                        if (ImGui::Selectable("Perspective", false))
                            c.camera.SetProjectionType(scene::Camera::ProjectionType::Perspective);
                        ImGui::EndCombo();
                    }

                    ImGui::Columns(1);

                    // Update the values if modified.
                    if (width != c.camera.GetWidth())
                        c.camera.SetWidth(width);
                    if (height != c.camera.GetHeight())
                        c.camera.SetHeight(height);
                    if (fov != c.camera.GetFOV())
                        c.camera.SetFOV(fov);
                    if (near_clip != c.camera.GetNearClip())
                        c.camera.SetNearClip(near_clip);
                    if (far_clip != c.camera.GetFarClip())
                        c.camera.SetFarClip(far_clip);
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<CameraComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<RigidBody2DComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Rigid Body 2D Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selectedEntity.entity.GetComponent<RigidBody2DComponent>();

                    // Body type.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Body type");
                        ImGui::NextColumn();

                        using BodyType = RigidBody2DComponent::BodyType;

                        std::string title;
                        switch (c.bodyType)
                        {
                            using enum BodyType;

                            case Static: title = "Static"; break;
                            case Dynamic: title = "Dynamic"; break;
                            case Kinematic: title = "Kinematic"; break;
                        }

                        if (ImGui::BeginCombo("###body_combo", title.c_str()))
                        {
                            if (ImGui::Selectable("Static", c.bodyType == BodyType::Static))
                                c.bodyType = BodyType::Static;
                            if (ImGui::Selectable("Dynamic", c.bodyType == BodyType::Dynamic))
                                c.bodyType = BodyType::Dynamic;
                            if (ImGui::Selectable("Kinematic", c.bodyType == BodyType::Kinematic))
                                c.bodyType = BodyType::Kinematic;
                            ImGui::EndCombo();
                        }
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Fixed rotation.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Fixed rotation");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###fixed_box", &c.fixedRotation);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Linear damping.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Linear damping");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###linear_drag", &c.linearDamping);
                        ImGui::Columns(1);
                    }

                    // Angular damping.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Angular damping");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###angular_drag", &c.angularDamping);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Enabled.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Simulate");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###enabled", &c.enabled);
                        ImGui::Columns(1);
                    }

                    // Gravity scale.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Gravity scale");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###gravity_drag", &c.gravityScale);
                        ImGui::Columns(1);
                    }
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<RigidBody2DComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<BoxCollider2DComponent>())
            {
                auto& c = d->selectedEntity.entity.GetComponent<BoxCollider2DComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Box Collider 2D Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    // Offset
                    {
                        SceneEditorView::DrawVec2Control("Offset", c.offset, d->columnWidth);
                    }

                    // Size
                    {
                        SceneEditorView::DrawVec2Control("Size", c.size, d->columnWidth);
                    }

                    // Physics material 2d.
                    {
                        DrawPhysicsMaterial2DControl(c.physicsMaterial, d->columnWidth);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<BoxCollider2DComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<CircleCollider2DComponent>())
            {
                auto& c = d->selectedEntity.entity.GetComponent<CircleCollider2DComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Circle Collider 2D Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    // Offset.
                    {
                        SceneEditorView::DrawVec2Control("Offset", c.offset, d->columnWidth);
                    }

                    // Radius.
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Radius");
                        ImGui::NextColumn();
                        ImGui::DragFloat("###radius_drag", &c.radius);
                        ImGui::Columns(1);
                    }

                    // Physics material 2d.
                    {
                        DrawPhysicsMaterial2DControl(c.physicsMaterial, d->columnWidth);
                    }
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<CircleCollider2DComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<GridRendererComponent>())
            {
                auto& c = d->selectedEntity.entity.GetComponent<GridRendererComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Grid Renderer Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    // Grid cell size
                    {
                        SceneEditorView::DrawVec2Control("Grid Cell Size", c.cellSize, d->columnWidth);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Gird colour
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
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
                    d->selectedEntity.entity.RemoveComponent<GridRendererComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<TilemapComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Tilemap Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selectedEntity.entity.GetComponent<TilemapComponent>();

                    // Sprite preview
                    {
                        // Texture prewview image
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->columnWidth);
                            ImGui::Text("Texture: ");
                            ImGui::NextColumn();

                            ImGui::BeginGroup();
                            if (c.sprite)
                                ImGui::Image(reinterpret_cast<ImTextureID>(c.sprite.GetTexture()->GetGlId()),
                                             { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
                            else
                                ImGui::Text("No bound texture.");

                            if (ImGui::Button("Load texture", { 100, 0 }))
                            {
                                nfdu8char_t*      outPath   = nullptr;
                                nfdu8filteritem_t filters[] = { { "Images", "png,jpg" } };
                                if (NFD_OpenDialogU8(&outPath, filters, 1, nullptr) == NFD_OKAY)
                                {
                                    std::filesystem::path relative_path =
                                        std::filesystem::relative(outPath, std::filesystem::current_path());
                                    NFD_FreePathU8(outPath);
                                    c.sprite.SetTexture(Resources::Load<gfx::Texture2D>(relative_path));
                                }
                            }
                            ImGui::EndGroup();
                            ImGui::Columns(1);
                            ImGui::Dummy(ImVec2(0.0f, 10.0f));
                        }

                        // Texture filter mode
                        if (c.sprite)
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->columnWidth);
                            ImGui::Text("Filter mode");
                            ImGui::NextColumn();

                            auto        texture      = c.sprite.GetTexture();
                            const char* preview_item = nullptr;
                            const auto& props        = texture->GetProperties();
                            switch (props.filterMode)
                            {
                                case opengl::TextureFilterMode::Linear: preview_item = "Linear"; break;
                                case opengl::TextureFilterMode::Nearest: preview_item = "Nearest"; break;
                            }
                            if (ImGui::BeginCombo("##texture_filter_mode", preview_item))
                            {
                                if (ImGui::Selectable("Nearest",
                                                      props.filterMode == opengl::TextureFilterMode::Nearest))
                                {
                                    if (props.filterMode != opengl::TextureFilterMode::Nearest)
                                    {
                                        auto new_props       = props;
                                        new_props.filterMode = opengl::TextureFilterMode::Nearest;
                                        texture->New(texture->GetFilePath(), new_props);
                                    }
                                }
                                if (ImGui::Selectable("Linear", props.filterMode == opengl::TextureFilterMode::Linear))
                                {
                                    if (props.filterMode != opengl::TextureFilterMode::Linear)
                                    {
                                        auto new_props       = props;
                                        new_props.filterMode = opengl::TextureFilterMode::Linear;
                                        texture->New(texture->GetFilePath(), new_props);
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
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Layer");
                        ImGui::NextColumn();
                        ImGui::DragInt("###layer_dragger", &c.currentLayer);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Gird size
                    {
                        auto new_grid_size = c.gridSize;
                        SceneEditorView::DrawVec2Control("Grid size", new_grid_size, d->columnWidth);
                        if (new_grid_size != c.gridSize)
                        {
                            c.gridSize                                                              = new_grid_size;
                            d->selectedEntity.entity.GetComponent<GridRendererComponent>().cellSize = new_grid_size;
                        }
                    }

                    // Tile size
                    {
                        SceneEditorView::DrawVec2Control("Tile size", c.tileSize, d->columnWidth);
                    }

                    if (ImGui::Button("Open tile pallete"))
                    {
                        auto& panel = this->AttachPanel<TilePalleteView>();
                        panel.SetEntity(d->selectedEntity.entity);
                        panel.Focus();
                    }
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<TilemapComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<TilesetAnimationComponent>())
            {
                auto& c = d->selectedEntity.entity.GetComponent<TilesetAnimationComponent>();
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Tileset Animation Componnet", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    // Sprite preview
                    {
                        // Texture prewview image
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->columnWidth);
                            ImGui::Text("Texture: ");
                            ImGui::NextColumn();

                            ImGui::BeginGroup();
                            if (c.sprite)
                                ImGui::Image(reinterpret_cast<ImTextureID>(c.sprite.GetTexture()->GetGlId()),
                                             { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
                            else
                                ImGui::Text("No bound texture.");

                            if (ImGui::Button("Load texture", { 100, 0 }))
                            {
                                nfdu8char_t*      outPath   = nullptr;
                                nfdu8filteritem_t filters[] = { { "Images", "png,jpg" } };
                                if (NFD_OpenDialogU8(&outPath, filters, 1, nullptr) == NFD_OKAY)
                                {
                                    std::filesystem::path relative_path =
                                        std::filesystem::relative(outPath, std::filesystem::current_path());
                                    NFD_FreePathU8(outPath);
                                    c.sprite.SetTexture(Resources::Load<gfx::Texture2D>(relative_path));
                                }
                            }
                            ImGui::EndGroup();
                            ImGui::Columns(1);
                            ImGui::Dummy(ImVec2(0.0f, 10.0f));
                        }

                        // Texture filter mode
                        if (c.sprite)
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->columnWidth);
                            ImGui::Text("Filter mode");
                            ImGui::NextColumn();

                            auto        texture      = c.sprite.GetTexture();
                            const char* preview_item = nullptr;
                            const auto& props        = texture->GetProperties();
                            switch (props.filterMode)
                            {
                                case opengl::TextureFilterMode::Linear: preview_item = "Linear"; break;
                                case opengl::TextureFilterMode::Nearest: preview_item = "Nearest"; break;
                            }
                            if (ImGui::BeginCombo("##texture_filter_mode", preview_item))
                            {
                                if (ImGui::Selectable("Nearest",
                                                      props.filterMode == opengl::TextureFilterMode::Nearest))
                                {
                                    if (props.filterMode != opengl::TextureFilterMode::Nearest)
                                    {
                                        auto new_props       = props;
                                        new_props.filterMode = opengl::TextureFilterMode::Nearest;
                                        texture->New(texture->GetFilePath(), new_props);
                                    }
                                }
                                if (ImGui::Selectable("Linear", props.filterMode == opengl::TextureFilterMode::Linear))
                                {
                                    if (props.filterMode != opengl::TextureFilterMode::Linear)
                                    {
                                        auto new_props       = props;
                                        new_props.filterMode = opengl::TextureFilterMode::Linear;
                                        texture->New(texture->GetFilePath(), new_props);
                                    }
                                }
                                ImGui::EndCombo();
                            }

                            ImGui::Columns(1);
                        }

                        // Add animation button
                        {
                            if (ImGui::Button("Add an animation"))
                            {
                                c.animations["animation #" + std::to_string(c.animations.size())] =
                                    TilesetAnimationComponent::Animation{};
                            }
                        }
                    }

                    for (auto& [name, anim] : c.animations)
                    {
                        if (ImGui::TreeNodeEx((name + "##tile_set_anim_" + name).c_str()))
                        {
                            // Starting tile
                            {
                                SceneEditorView::DrawVec2Control("Starting tile", anim.startingTile, d->columnWidth);
                            }

                            // Frame count
                            {
                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->columnWidth);
                                ImGui::Text("Frame count");
                                ImGui::NextColumn();
                                ImGui::DragInt(("###drag_int_" + name).c_str(),
                                               reinterpret_cast<i32*>(&anim.frameCount), 1.0f, 0);
                                ImGui::Columns(1);
                            }

                            // Frame rate
                            {
                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->columnWidth);
                                ImGui::Text("Frame rate");
                                ImGui::NextColumn();
                                ImGui::DragFloat(("###drag_f32_" + name).c_str(), &anim.frameRate, 1.0f, 0);
                                ImGui::Columns(1);
                            }

                            // Animation preview
                            {
                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->columnWidth);
                                ImGui::Text("Texture: ");
                                ImGui::NextColumn();

                                ImGui::BeginGroup();
                                if (c.sprite)
                                    ImGui::Image(reinterpret_cast<ImTextureID>(c.sprite.GetTexture()->GetGlId()),
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
                    d->selectedEntity.entity.RemoveComponent<TilesetAnimationComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<AudioSourceComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Audio Source Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selectedEntity.entity.GetComponent<AudioSourceComponent>();

                    // Event path (searchable dropdown)
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Event path");
                        ImGui::NextColumn();

                        static std::string              search_filter;
                        static std::vector<std::string> cached_events;
                        static bool                     needs_refresh = true;

                        const char* preview = c.eventPath.empty() ? "Select an event..." : c.eventPath.c_str();
                        if (ImGui::BeginCombo("###audio_event_path", preview))
                        {
                            if (needs_refresh)
                            {
                                cached_events = ax::AudioManager::GetAllEventPaths();
                                needs_refresh = false;
                            }

                            ImGui::InputTextWithHint("###event_search", "Search...", &search_filter);
                            ImGui::Separator();

                            for (const auto& ev : cached_events)
                            {
                                if (!search_filter.empty() && ev.find(search_filter) == std::string::npos)
                                    continue;

                                const bool is_selected = (ev == c.eventPath);
                                if (ImGui::Selectable(ev.c_str(), is_selected))
                                {
                                    c.eventPath = ev;
                                    search_filter.clear();
                                }
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }

                            ImGui::EndCombo();
                        }
                        else
                        {
                            needs_refresh = true;
                            search_filter.clear();
                        }

                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Sound path
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Sound file");
                        ImGui::NextColumn();

                        static std::string sound_path_str;
                        sound_path_str = c.soundPath.string();
                        ImGui::InputText("###audio_sound_path", &sound_path_str);
                        c.soundPath = sound_path_str;

                        if (ImGui::Button("Browse##audio_browse", { 100, 0 }))
                        {
                            nfdu8char_t*      outPath   = nullptr;
                            nfdu8filteritem_t filters[] = { { "Audio Files", "wav,ogg,mp3,flac" } };
                            if (NFD_OpenDialogU8(&outPath, filters, 1, nullptr) == NFD_OKAY)
                            {
                                c.soundPath = std::filesystem::relative(outPath, std::filesystem::current_path());
                                NFD_FreePathU8(outPath);
                            }
                        }
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Volume
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Volume");
                        ImGui::NextColumn();
                        ImGui::SliderFloat("###audio_volume", &c.volume, 0.0f, 2.0f);
                        ImGui::Columns(1);
                    }

                    // Pitch
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Pitch");
                        ImGui::NextColumn();
                        ImGui::SliderFloat("###audio_pitch", &c.pitch, 0.1f, 4.0f);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // Loop
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Loop");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###audio_loop", &c.loop);
                        ImGui::Columns(1);
                    }

                    // Play on start
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("Play on start");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###audio_play_on_start", &c.playOnStart);
                        ImGui::Columns(1);
                    }

                    ImGui::Dummy(ImVec2(0.0f, 10.0f));

                    // 3D Audio
                    {
                        ImGui::Columns(2);
                        ImGui::SetColumnWidth(0, d->columnWidth);
                        ImGui::Text("3D Audio");
                        ImGui::NextColumn();
                        ImGui::Checkbox("###audio_is_3d", &c.is3D);
                        ImGui::Columns(1);
                    }

                    if (c.is3D)
                    {
                        // Min distance
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->columnWidth);
                            ImGui::Text("Min distance");
                            ImGui::NextColumn();
                            ImGui::DragFloat("###audio_min_dist", &c.minDistance, 0.1f, 0.0f, c.maxDistance);
                            ImGui::Columns(1);
                        }

                        // Max distance
                        {
                            ImGui::Columns(2);
                            ImGui::SetColumnWidth(0, d->columnWidth);
                            ImGui::Text("Max distance");
                            ImGui::NextColumn();
                            ImGui::DragFloat("###audio_max_dist", &c.maxDistance, 1.0f, c.minDistance, 10000.0f);
                            ImGui::Columns(1);
                        }
                    }

                    // Event parameters
                    if (!c.eventPath.empty())
                    {
                        ImGui::Dummy(ImVec2(0.0f, 10.0f));

                        static std::vector<ax::EventParameterInfo> cached_params;
                        static std::string                         cached_params_event;

                        // Refresh when the event path changes.
                        if (cached_params_event != c.eventPath)
                        {
                            cached_params       = ax::AudioManager::GetEventParameters(c.eventPath);
                            cached_params_event = c.eventPath;

                            // Populate defaults for parameters not yet in the map.
                            for (const auto& p : cached_params)
                            {
                                if (!c.parameters.contains(p.name))
                                    c.parameters[p.name] = p.defaultValue;
                            }
                        }

                        if (!cached_params.empty() &&
                            ImGui::TreeNodeEx("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            for (const auto& p : cached_params)
                            {
                                auto& val = c.parameters[p.name];
                                ImGui::Columns(2);
                                ImGui::SetColumnWidth(0, d->columnWidth);
                                ImGui::Text("%s", p.name.c_str());
                                ImGui::NextColumn();
                                ImGui::SliderFloat(
                                    ("###param_" + p.name).c_str(), &val, p.minimum, p.maximum);
                                ImGui::Columns(1);
                            }
                            ImGui::TreePop();
                        }
                    }
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<AudioSourceComponent>();
            }
            if (d->selectedEntity.entity.HasComponent<AudioListenerComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                bool remove = false;
                bool open = ImGui::CollapsingHeader("Audio Listener Component", ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Remove Component"))
                        remove = true;
                    ImGui::EndPopup();
                }
                if (open)
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    ImGui::Text("Position and orientation are taken from the Transform Component.");
                }
                if (remove)
                    d->selectedEntity.entity.RemoveComponent<AudioListenerComponent>();
            }
        }

        ImGui::End();
    }

    void PropertiesView::DrawPhysicsMaterial2DControl(phys::PhysicsMaterial2D& mat, const f32 columnWidth) noexcept
    {
        if (ImGui::TreeNodeEx("Physics Material 2D", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Density
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("Density");
                ImGui::NextColumn();
                ImGui::DragFloat("###density_drag", &mat.density);
                ImGui::Columns(1);
            }

            // Friction
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("Friction");
                ImGui::NextColumn();
                ImGui::DragFloat("###friction_drag", &mat.friction);
                ImGui::Columns(1);
            }

            // Restitution
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("Restitution");
                ImGui::NextColumn();
                ImGui::DragFloat("###restitution_drag", &mat.restitution);
                ImGui::Columns(1);
            }

            // Restitution threshold
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("Restitution threshold");
                ImGui::NextColumn();
                ImGui::DragFloat("###threshold_drag", &mat.restitutionThreshold);
                ImGui::Columns(1);
            }

            ImGui::TreePop();
        }
    }
} // namespace codex::editor
