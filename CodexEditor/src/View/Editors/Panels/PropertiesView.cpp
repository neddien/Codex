#include "PropertiesView.h"

#include "../SceneEditorView.h"
#include "TilePalleteView.h"

#include <tinyfiledialogs.h>

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
                if (ImGui::CollapsingHeader("C++ Script Component", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Dummy(ImVec2(0.0f, 10.0f));
                    auto& c = d->selectedEntity.entity.GetComponent<NativeBehaviourComponent>();

                    // Attached scripts.
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, d->columnWidth);
                    ImGui::Text("Attached behaviours: ");
                    ImGui::NextColumn();

                    if (ImGui::BeginCombo("###script_combo", "Select a script"))
                    {
                        for (const auto& file : d->scripts)
                        {
                            const auto& file_name = file.GetSourceFile().filename().string();

                            for (const auto& script : file.GetClasses())
                            {
                                const auto  script_name = script.name;
                                const auto& behaviours  = c.GetBehaviours();

                                const auto it =
                                    std::find_if(behaviours.cbegin(), behaviours.cend(),
                                                 [&script_name](const auto& e) { return e.first == script_name; });
                                if (it == behaviours.cend())
                                {
                                    if (ImGui::Selectable(script_name.c_str(), false))
                                    {
                                        auto* behaviour = d->activeScene.Lock()->CreateBehaviour(
                                            script_name.c_str(), d->selectedEntity.entity);
                                        c.Attach(std::move(behaviour));
                                    }
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Columns(1);

                    // Display attached scripts and their serialized fields.
                    auto&                  behaviours = c.GetBehaviours();
                    std::list<std::string> possible_scripts_to_detach;
                    for (auto it = behaviours.begin(); it != behaviours.end(); ++it)
                    {
                        auto& [k, v] = *it;

                        // TODO: Come back
                        /*
                        const auto& j = v->GetSerializedData();
                        if (j.empty())
                            v->Serialize();

                        const auto& klass = j.begin();
                        if (ImGui::CollapsingHeader(klass.key().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            if (ImGui::Button("Detach script"))
                            {
                                possible_scripts_to_detach.push_back(it->first);
                                continue;
                            }

                            if (klass.value().contains("Fields"))
                            {
                                for (const auto& field : klass.value()["Fields"].items())
                                {
                                    const std::string& field_name = field.key();
                                    const auto         field_id   = fmt::format("##{}", field_name);
                                    const rf::RFType   field_type = field.value().at("Type");

                                    ImGui::Columns(2);
                                    ImGui::SetColumnWidth(0, d->columnWidth);
                                    ImGui::Text("%s", field_name.c_str());
                                    ImGui::NextColumn();

                                    object field_ptr = v->GetField(field_name);
                                    switch (field_type)
                                    {
                                        using enum rf::RFType;

                                        case I32:
                                        case U32: {
                                            ImGui::DragInt(field_id.c_str(), (i32*)field_ptr);
                                            break;
                                        }
                                        case F32:
                                        case F64:
                                        case F128: {
                                            ImGui::DragFloat(field_id.c_str(), (f32*)field_ptr);
                                            break;
                                        }
                                        case StdString: {
                                            ImGui::InputText(field_id.c_str(), (std::string*)field_ptr);
                                            break;
                                        }
                                        case Boolean: {
                                            ImGui::Checkbox(field_id.c_str(), (bool*)field_ptr);
                                            break;
                                        }
                                        case Vector2f: {
                                            SceneEditorView::DrawVec2Control(
                                                field_id.c_str(), *(math::Vector2f*)field_ptr, d->columnWidth);
                                            break;
                                        }
                                        case Vector3f: {
                                            SceneEditorView::DrawVec3Control(
                                                field_id.c_str(), *(math::Vector3f*)field_ptr, d->columnWidth);
                                            break;
                                        }
                                        default: break; // cx_throw(CodexException, "Should not happen."); break;
                                    }
                                    ImGui::Columns(1);
                                }
                            }
                        }
                        */
                    }
                    for (const auto& e : possible_scripts_to_detach)
                        c.Detach(e);
                }
            }
            if (d->selectedEntity.entity.HasComponent<SpriteRendererComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Sprite Renderer Component", ImGuiTreeNodeFlags_DefaultOpen))
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
                            const char* filters[] = { "*.png", "*.jpg" };
                            const char* file =
                                tinyfd_openFileDialog("Load texture file.", nullptr, 2, filters, nullptr, 0);
                            if (file)
                            {
                                std::filesystem::path relative_path =
                                    std::filesystem::relative(file, std::filesystem::current_path());
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
            }
            if (d->selectedEntity.entity.HasComponent<CameraComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen))
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
            }
            if (d->selectedEntity.entity.HasComponent<RigidBody2DComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Rigid Body 2D Component", ImGuiTreeNodeFlags_DefaultOpen))
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
            }
            if (d->selectedEntity.entity.HasComponent<BoxCollider2DComponent>())
            {
                auto& c = d->selectedEntity.entity.GetComponent<BoxCollider2DComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Box Collider 2D Component", ImGuiTreeNodeFlags_DefaultOpen))
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
            }
            if (d->selectedEntity.entity.HasComponent<CircleCollider2DComponent>())
            {
                auto& c = d->selectedEntity.entity.GetComponent<CircleCollider2DComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Circle Collider 2D Component", ImGuiTreeNodeFlags_DefaultOpen))
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
            }
            if (d->selectedEntity.entity.HasComponent<GridRendererComponent>())
            {
                auto& c = d->selectedEntity.entity.GetComponent<GridRendererComponent>();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Grid Renderer Component", ImGuiTreeNodeFlags_DefaultOpen))
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
            }
            if (d->selectedEntity.entity.HasComponent<TilemapComponent>())
            {
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::CollapsingHeader("Tilemap Component", ImGuiTreeNodeFlags_DefaultOpen))
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
                                const char* filters[] = { "*.png", "*.jpg" };
                                const char* file =
                                    tinyfd_openFileDialog("Load texture file.", nullptr, 2, filters, nullptr, 0);
                                if (file)
                                {
                                    std::filesystem::path relative_path =
                                        std::filesystem::relative(file, std::filesystem::current_path());
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
            }
            if (d->selectedEntity.entity.HasComponent<TilesetAnimationComponent>())
            {
                auto& c = d->selectedEntity.entity.GetComponent<TilesetAnimationComponent>();
                if (ImGui::CollapsingHeader("Tileset Animation Componnet", ImGuiTreeNodeFlags_DefaultOpen))
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
                                const char* filters[] = { "*.png", "*.jpg" };
                                const char* file =
                                    tinyfd_openFileDialog("Load texture file.", nullptr, 2, filters, nullptr, 0);
                                if (file)
                                {
                                    std::filesystem::path relative_path =
                                        std::filesystem::relative(file, std::filesystem::current_path());
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
