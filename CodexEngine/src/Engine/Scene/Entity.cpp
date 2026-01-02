#include "Public/Entity.h"
#include "Public/Entity.inl"

#include "Public/Components.inl"

namespace codex {
    UUID Entity::GetUUID() const noexcept
    {
        return GetComponent<IDComponent>().uuid;
    }

    TransformComponent& Entity::GetTransform() noexcept
    {
        return GetComponent<TransformComponent>();
    }

    const TransformComponent& Entity::GetTransform() const noexcept
    {
        return GetComponent<TransformComponent>();
    }

    /*
    void to_json(nlohmann::ordered_json& j, const Entity& entity)
    {
        nlohmann::ordered_json jentity;
        jentity["UUID"]       = entity.GetComponent<IDComponent>().uuid;
        jentity["Components"] = nlohmann::ordered_json::array();

        if (entity.HasComponent<TagComponent>())
        {
            const auto&            c = entity.GetComponent<TagComponent>();
            nlohmann::ordered_json jc;
            jc["TagComponent"]["tag"] = c.tag;
            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<TransformComponent>())
        {
            const auto&            c = entity.GetComponent<TransformComponent>();
            nlohmann::ordered_json jc;
            jc["TransformComponent"] = { { "position", c.position }, { "rotation", c.rotation }, { "scale", c.scale } };
            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<SpriteRendererComponent>())
        {
            const auto&            c = entity.GetComponent<SpriteRendererComponent>();
            nlohmann::ordered_json jc;
            jc["SpriteRendererComponent"]["m_Sprite"] = c.GetSprite();
            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<NativeBehaviourComponent>())
        {
            auto&                  c = entity.GetComponent<NativeBehaviourComponent>();
            nlohmann::ordered_json jc;

            std::list<std::string_view> scripts;
            std::for_each(c.GetBehaviours().begin(), c.GetBehaviours().end(),
                          [&scripts](const auto& e) { scripts.push_back(e.first); });
            jc["NativeBehaviourComponent"]["AttachedScripts"] = scripts;
            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<CameraComponent>())
        {
            auto&                  c = entity.GetComponent<CameraComponent>();
            nlohmann::ordered_json jc;

            jc["CameraComponent"]["Primary"]                  = c.primary;
            jc["CameraComponent"]["Camera"]["Width"]          = c.camera.GetWidth();
            jc["CameraComponent"]["Camera"]["Height"]         = c.camera.GetHeight();
            jc["CameraComponent"]["Camera"]["NearClip"]       = c.camera.GetNearClip();
            jc["CameraComponent"]["Camera"]["FarClip"]        = c.camera.GetFarClip();
            jc["CameraComponent"]["Camera"]["FOV"]            = c.camera.GetFOV();
            jc["CameraComponent"]["Camera"]["ProjectionType"] = c.camera.GetProjectionType();

            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<RigidBody2DComponent>())
        {
            auto&                  c = entity.GetComponent<RigidBody2DComponent>();
            nlohmann::ordered_json jc;

            jc["RigidBody2DComponent"]["BodyType"]       = c.bodyType;
            jc["RigidBody2DComponent"]["FixedRotation"]  = c.fixedRotation;
            jc["RigidBody2DComponent"]["LinearDamping"]  = c.linearDamping;
            jc["RigidBody2DComponent"]["AngularDamping"] = c.angularDamping;
            jc["RigidBody2DComponent"]["HighVelocity"]   = c.highVelocity;
            jc["RigidBody2DComponent"]["Enabled"]        = c.enabled;
            jc["RigidBody2DComponent"]["GravityScale"]   = c.gravityScale;

            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<BoxCollider2DComponent>())
        {
            auto&                  c = entity.GetComponent<BoxCollider2DComponent>();
            nlohmann::ordered_json jc;

            jc["BoxCollider2DComponent"]["Offset"]            = c.offset;
            jc["BoxCollider2DComponent"]["Size"]              = c.size;
            jc["BoxCollider2DComponent"]["PhysicsMaterial2D"] = c.physicsMaterial;

            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<CircleCollider2DComponent>())
        {
            auto&                  c = entity.GetComponent<CircleCollider2DComponent>();
            nlohmann::ordered_json jc;

            jc["CircleCollider2DComponent"]["Offset"]            = c.offset;
            jc["CircleCollider2DComponent"]["Radius"]            = c.radius;
            jc["CircleCollider2DComponent"]["PhysicsMaterial2D"] = c.physicsMaterial;

            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<GridRendererComponent>())
        {
            auto&                  c = entity.GetComponent<GridRendererComponent>();
            nlohmann::ordered_json jc;

            jc["GridRendererComponent"]["CellSize"] = c.cellSize;
            jc["GridRendererComponent"]["Colour"]   = c.colour;

            jentity["Components"].push_back(jc);
        }
        if (entity.HasComponent<TilemapComponent>())
        {
            auto&                  c = entity.GetComponent<TilemapComponent>();
            nlohmann::ordered_json jc;

            jc["TilemapComponent"]["Sprite"]       = c.sprite;
            jc["TilemapComponent"]["Tiles"]        = c.tiles;
            jc["TilemapComponent"]["GridSize"]     = c.gridSize;
            jc["TilemapComponent"]["TileSize"]     = c.tileSize;
            jc["TilemapComponent"]["CurrentLayer"] = c.currentLayer;

            jentity["Components"].push_back(jc);
        }
        j = jentity;
    }

    void from_json(const nlohmann::ordered_json& j, Entity& entity)
    {
        auto is_component = [&](auto& j, const char* component) -> nlohmann::ordered_json
        {
            if (j.is_object() && j.count(component))
                return j.at(component);
            return nlohmann::ordered_json{};
        };

        entity.GetComponent<IDComponent>().uuid = j.at("UUID").get<UUID>();

        const auto& components = j.at("Components");
        for (const auto& c : components)
        {
            if (auto cj = is_component(c, "TagComponent"); !cj.empty())
            {
                auto& nc = entity.GetComponent<TagComponent>();
                cj.at("tag").get_to(nc.tag);
            }
            else if (auto cj = is_component(c, "TransformComponent"); !cj.empty())
            {
                auto& nc = entity.GetComponent<TransformComponent>();
                cj.at("position").get_to(nc.position);
                cj.at("rotation").get_to(nc.rotation);
                cj.at("scale").get_to(nc.scale);
            }
            else if (auto cj = is_component(c, "SpriteRendererComponent"); !cj.empty())
            {
                Sprite s;
                cj.at("m_Sprite").get_to(s);
                entity.AddComponent<SpriteRendererComponent>(s);
            }
            else if (auto cj = is_component(c, "NativeBehaviourComponent"); !cj.empty())
            {
                // TODO: Here we boldly assume that Native Scripting is being used so we call CreateBehaviour()
                // on scene without worrying, but in the future we should worry.
                auto& nc = entity.AddComponent<NativeBehaviourComponent>();
                for (const auto& e : cj.at("AttachedScripts"))
                {
                    const auto class_name = e.get<std::string>();
                    if (Scene::IsScriptModuleLoaded())
                    {
                        auto* class_instance = entity.m_Scene->CreateBehaviour(class_name.c_str(), entity);
                        class_instance->SetOwner(entity);
                        nc.Attach(std::move(class_instance));
                    }
                }
            }
            else if (auto cj = is_component(c, "CameraComponent"); !cj.empty())
            {
                auto&       cc          = entity.AddComponent<CameraComponent>();
                const auto& camera_prop = cj.at("Camera");
                cc.primary              = cj.at("Primary");
                cc.camera.SetWidth(camera_prop.at("Width"));
                cc.camera.SetHeight(camera_prop.at("Height"));
                cc.camera.SetNearClip(camera_prop.at("NearClip"));
                cc.camera.SetFarClip(camera_prop.at("FarClip"));
                cc.camera.SetFOV(camera_prop.at("FOV"));
                cc.camera.SetProjectionType(camera_prop.at("ProjectionType"));
            }
            else if (auto cj = is_component(c, "RigidBody2DComponent"); !cj.empty())
            {
                auto& cc = entity.AddComponent<RigidBody2DComponent>();

                cj.at("BodyType").get_to(cc.bodyType);
                cj.at("FixedRotation").get_to(cc.fixedRotation);
                cj.at("LinearDamping").get_to(cc.linearDamping);
                cj.at("AngularDamping").get_to(cc.angularDamping);
                cj.at("HighVelocity").get_to(cc.highVelocity);
                cj.at("Enabled").get_to(cc.enabled);
                cj.at("GravityScale").get_to(cc.gravityScale);
            }
            else if (auto cj = is_component(c, "BoxCollider2DComponent"); !cj.empty())
            {
                auto& cc = entity.AddComponent<BoxCollider2DComponent>();

                cj.at("Offset").get_to(cc.offset);
                cj.at("Size").get_to(cc.size);
                cj.at("PhysicsMaterial2D").get_to(cc.physicsMaterial);
            }
            else if (auto cj = is_component(c, "CircleCollider2DComponent"); !cj.empty())
            {
                auto& cc = entity.AddComponent<CircleCollider2DComponent>();

                cj.at("Offset").get_to(cc.offset);
                cj.at("Radius").get_to(cc.radius);
                cj.at("PhysicsMaterial2D").get_to(cc.physicsMaterial);
            }
            else if (auto cj = is_component(c, "GridRendererComponent"); !cj.empty())
            {
                auto& cc = entity.AddComponent<GridRendererComponent>();

                cj.at("CellSize").get_to(cc.cellSize);
                cj.at("Colour").get_to(cc.colour);
            }
            else if (auto cj = is_component(c, "TilemapComponent"); !cj.empty())
            {
                auto& cc = entity.AddComponent<TilemapComponent>();

                cj.at("Sprite").get_to(cc.sprite);
                cj.at("Tiles").get_to(cc.tiles);
                cj.at("GridSize").get_to(cc.gridSize);
                cj.at("TileSize").get_to(cc.tileSize);
                cj.at("CurrentLayer").get_to(cc.currentLayer);
            }
        }
    }
    */
} // namespace codex
