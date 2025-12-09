#include "Public/Serializer.h"

#include <sdafx.h>

namespace codex {
    namespace stdfs = std::filesystem;

    /*
    void Serializer::SerializeScene(const stdfs::path path, const Scene& scene)
    {
        std::ofstream fs{ path };
        if (fs.is_open())
        {
            nlohmann::ordered_json j = scene;
            fs << std::setw(4) << j;
            fs.close();
        }
        else
        {
            // throw exception
        }
    }

    void Serializer::DeserializeScene(const stdfs::path path, Scene& scene)
    {
        std::ifstream fs{ path };
        if (fs.is_open())
        {
            std::string            str((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
            nlohmann::ordered_json j = nlohmann::json::parse(str);

            std::vector<Entity> entities;
            j.at("Name").get_to(scene.m_Name);
            if (j.contains("Entities") && j["Entities"].is_array())
            {
                for (const auto& e : j["Entities"])
                {
                    Entity entt = scene.CreateEntity();
                    from_json(e, entt);
                }
            }
        }
        else
        {
            // throw exception
        }
    }
    */

} // namespace codex
