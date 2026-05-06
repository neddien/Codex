#pragma once

#include "constants.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace codex::opengl {
    class CODEX_API Shader
    {
    public:
        Shader(std::string source, std::string_view version = "330 core");
        ~Shader();

    public:
        [[nodiscard]] std::string_view version() const noexcept { return version_; }

    public:
        void bind() const;
        void unbind() const;
        void set_uniform_1i(const char* name, const i32 value);
        void set_uniform_1i_arr(const char* name, const u32 count, const i32* value);
        void set_uniform_1f(const char* name, const f32 value);
        void set_uniform_2f(const char* name, const f32 a0, const f32 a1);
        void set_uniform_4f(const char* name, const f32 a0, const f32 a1, const f32 a2, const f32 a3);
        void set_uniform_3f(const char* name, const f32 a0, const f32 a1, const f32 a2);
        void set_uniform_mat4f(const char* name, const glm::mat4& matrix);
        void set_uniform_mat4f(const char* name, const f32* matrix);
        bool compile_shader(
            const std::initializer_list<std::pair<std::string_view, std::string_view>> compile_definitions = {});

    private:
        void    parse_shader_source();
        int32_t get_uniform_location(const char* name);

    private:
        u32                                      renderer_id_;
        std::string                              source_;
        std::string                              vertex_source_;
        std::string                              fragment_source_;
        u32                                      vertex_shader_id_;
        u32                                      fragment_shader_id_;
        std::unordered_map<std::string, int32_t> uniform_locations_;
        std::string                              version_;
    };
} // namespace codex::opengl
