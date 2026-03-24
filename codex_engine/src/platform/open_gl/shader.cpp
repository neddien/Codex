#include "shader.h"
#include "renderer.h"

#include <fstream>
#include <glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

namespace codex::opengl {
    Shader::Shader(std::filesystem::path file_path, const std::string_view version)
        : file_path_(std::move(file_path))
        , version_(version)
    {
        GL_Call(renderer_id_ = glCreateProgram());
    }

    Shader::~Shader()
    {
        GL_Call(glUseProgram(0));
        GL_Call(glDeleteProgram(renderer_id_));
        GL_Call(glDeleteShader(vertex_shader_id_));
        GL_Call(glDeleteShader(fragment_shader_id_));
    }

    void Shader::bind() const
    {
        GL_Call(glUseProgram(renderer_id_));
    }

    void Shader::unbind() const
    {
        GL_Call(glUseProgram(0));
    }

    void Shader::set_uniform_1i(const char* name, const int32_t value)
    {
        GL_Call(glUniform1i(get_uniform_location(name), value));
    }

    void Shader::set_uniform_1i_arr(const char* name, const u32 count, const i32* value)
    {
        glUniform1iv(get_uniform_location(name), count, value);
    }

    void Shader::set_uniform_1f(const char* name, const float value)
    {
        GL_Call(glUniform1f(get_uniform_location(name), value));
    }

    void Shader::set_uniform_2f(const char* name, const float a0, const float a1)
    {
        GL_Call(glUniform2f(get_uniform_location(name), a0, a1));
    }

    void Shader::set_uniform_4f(const char* name, const float a0, const float a1, const float a2, const float a3)
    {
        GL_Call(glUniform4f(get_uniform_location(name), a0, a1, a2, a3));
    }

    void Shader::set_uniform_3f(const char* name, const float a0, const float a1, const float a2)
    {
        GL_Call(glUniform3f(get_uniform_location(name), a0, a1, a2));
    }

    void Shader::set_uniform_mat4f(const char* name, const glm::mat4& matrix)
    {
        GL_Call(glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(matrix)));
    }

    void Shader::set_uniform_mat4f(const char* name, const float* matrix)
    {
        GL_Call(glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, matrix));
    }

    void Shader::parse_shader_file()
    {
        std::ifstream fs(file_path_);

        if (!fs.is_open() || !fs.good())
        {
            std::cerr << "[OpenGL]::[ERROR] >> Failed to open file '" << file_path_ << "'!" << std::endl;
            throw std::runtime_error("[OpenGL]::[ERROR] >> Failed to open file shader file!");
        }

        std::string line;
        bool        vertex = false, fragment = false;
        while (std::getline(fs, line))
        {
            if (line == "#shader_type vertex")
            {
                vertex   = true;
                fragment = false;
            } else if (line == "#shader_type fragment")
            {
                fragment = true;
                vertex   = false;
            } else if (vertex)
                vertex_source_.append(line + '\n');
            else if (fragment)
                fragment_source_.append(line + '\n');
        }
        fs.close();
    }

    bool Shader::compile_shader(
        const std::initializer_list<std::pair<std::string_view, std::string_view>> compile_definitions)
    {
        parse_shader_file();

        std::string definitions;
        for (const auto& def : compile_definitions)
        {
            std::string str_def = "#define ";
            str_def.append(def.first);
            str_def.append(1, ' ');
            str_def.append(def.second);
            str_def.append(1, '\n');
            definitions.append(str_def);
        }

        vertex_source_.insert(0, definitions);
        fragment_source_.insert(0, definitions);

        vertex_source_.insert(0, "#version " + version_ + "\n");
        fragment_source_.insert(0, "#version " + version_ + "\n");

        GL_Call(vertex_shader_id_ = glCreateShader(GL_VERTEX_SHADER));
        GL_Call(fragment_shader_id_ = glCreateShader(GL_FRAGMENT_SHADER));

        const char* c_vertex_source   = vertex_source_.c_str();
        const char* c_fragment_source = fragment_source_.c_str();

        GL_Call(glShaderSource(vertex_shader_id_, 1, &c_vertex_source, nullptr));
        GL_Call(glShaderSource(fragment_shader_id_, 1, &c_fragment_source, nullptr));

        GL_Call(glCompileShader(vertex_shader_id_));
        int v_compile_status;
        glGetShaderiv(vertex_shader_id_, GL_COMPILE_STATUS, &v_compile_status);
        if (!v_compile_status)
        {
            int length;
            glGetShaderiv(vertex_shader_id_, GL_INFO_LOG_LENGTH, &length);
            char* err_msg = (char*)alloca(length);
            glGetShaderInfoLog(vertex_shader_id_, length, nullptr, err_msg);
            std::cout << "[OpenGL]::[ERROR] >> Failed to compile Vertex Shader!\n\tGLMSG > " << err_msg << std::endl;
        }
        GL_Call(glCompileShader(fragment_shader_id_));
        int f_compile_status;
        glGetShaderiv(fragment_shader_id_, GL_COMPILE_STATUS, &f_compile_status);
        if (!f_compile_status)
        {
            int length;
            glGetShaderiv(fragment_shader_id_, GL_INFO_LOG_LENGTH, &length);
            char* err_msg = (char*)alloca(length);
            glGetShaderInfoLog(fragment_shader_id_, length, nullptr, err_msg);
            std::cout << "[OpenGL]::[ERROR] >> Failed to compile Fragment Shader!\n\tGLMSG > " << err_msg << std::endl;
        }

        GL_Call(glAttachShader(renderer_id_, vertex_shader_id_));
        GL_Call(glAttachShader(renderer_id_, fragment_shader_id_));

        GL_Call(glLinkProgram(renderer_id_));
        int p_link_status;
        glGetProgramiv(renderer_id_, GL_LINK_STATUS, &p_link_status);
        if (!p_link_status)
        {
            int length;
            glGetProgramiv(renderer_id_, GL_INFO_LOG_LENGTH, &length);
            char* err_msg = (char*)alloca(length);
            glGetProgramInfoLog(renderer_id_, length, nullptr, err_msg);
            std::cout << "[OpenGL]::[ERROR] >> Failed to link shaders!\n\tGLMSG > " << err_msg << std::endl;
        }
        return true;
    }

    int32_t Shader::get_uniform_location(const char* name)
    {
        int32_t     location;
        const auto& found = uniform_locations_.find(name);
        if (found != uniform_locations_.end())
            location = found->second;
        else {
            GL_Call(location = glGetUniformLocation(renderer_id_, name));
            uniform_locations_[name] = location;
            if (location == -1)
                std::cout << "[OpenGL]::[WARNING] >> Uniform '" << name << "' does not exist!" << std::endl;
        }
        return location;
    }
} // namespace codex::opengl
