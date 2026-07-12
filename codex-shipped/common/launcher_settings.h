#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include <engine/core/window.h>

namespace codex::shipped {
    namespace stdfs = std::filesystem;

    struct VideoSettings
    {
        static constexpr u32 CurrentVersion = 1;

        u32         version      = CurrentVersion;
        std::string display_mode = "windowed";
        i32         width        = 1280;
        i32         height       = 720;
        i32         monitor      = 0;
        i32         refresh_rate = 60;
        u32         frame_cap    = 300;
        bool        vsync        = true;
        bool        resizable    = true;

        void validate() noexcept
        {
            version      = CurrentVersion;
            width        = std::clamp(width, 640, 16384);
            height       = std::clamp(height, 480, 16384);
            monitor      = std::max(monitor, 0);
            refresh_rate = std::clamp(refresh_rate, 24, 1000);
            frame_cap    = std::min(frame_cap, 1000u);
            if (display_mode != "windowed" && display_mode != "borderless" && display_mode != "fullscreen")
                display_mode = "windowed";
        }

        [[nodiscard]] static VideoSettings from(const VideoProperties& video)
        {
            VideoSettings result;
            result.width     = video.window_width;
            result.height    = video.window_height;
            result.frame_cap = video.frame_cap;
            result.vsync     = video.vsync;
            result.resizable = !!(video.window_flags & WindowFlags::Resizable);
            if (video.window_flags & WindowFlags::FullScreen)
                result.display_mode = "fullscreen";
            else if (video.borderless || (video.window_flags & WindowFlags::Borderless))
                result.display_mode = "borderless";
            result.validate();
            return result;
        }

        void apply(VideoProperties& video) const noexcept
        {
            VideoSettings value = *this;
            value.validate();
            video.window_width  = value.width;
            video.window_height = value.height;
            video.frame_cap     = value.frame_cap;
            video.vsync         = value.vsync;
            video.borderless    = value.display_mode == "borderless";

            video.window_flags = WindowFlags::Visible | WindowFlags::PositionCentre;
            if (value.resizable && value.display_mode == "windowed")
                video.window_flags |= WindowFlags::Resizable;
            if (value.display_mode == "borderless")
                video.window_flags |= WindowFlags::Borderless;
            if (value.display_mode == "fullscreen")
                video.window_flags |= WindowFlags::FullScreen;
        }
    };

    struct LauncherSettings
    {
        static constexpr u32 CurrentVersion = 1;

        u32  version    = CurrentVersion;
        bool auto_start = false;
    };

    inline void to_json(nlohmann::json& json, const VideoSettings& value)
    {
        json = nlohmann::json{
            { "version", value.version },           { "display_mode", value.display_mode },
            { "width", value.width },               { "height", value.height },
            { "monitor", value.monitor },           { "refresh_rate", value.refresh_rate },
            { "frame_cap", value.frame_cap },       { "vsync", value.vsync },
            { "resizable", value.resizable },
        };
    }

    inline void from_json(const nlohmann::json& json, VideoSettings& value)
    {
        if (json.contains("version"))
            json.at("version").get_to(value.version);
        if (json.contains("display_mode"))
            json.at("display_mode").get_to(value.display_mode);
        if (json.contains("width"))
            json.at("width").get_to(value.width);
        if (json.contains("height"))
            json.at("height").get_to(value.height);
        if (json.contains("monitor"))
            json.at("monitor").get_to(value.monitor);
        if (json.contains("refresh_rate"))
            json.at("refresh_rate").get_to(value.refresh_rate);
        if (json.contains("frame_cap"))
            json.at("frame_cap").get_to(value.frame_cap);
        if (json.contains("vsync"))
            json.at("vsync").get_to(value.vsync);
        if (json.contains("resizable"))
            json.at("resizable").get_to(value.resizable);
        value.validate();
    }

    inline void to_json(nlohmann::json& json, const LauncherSettings& value)
    {
        json = nlohmann::json{ { "version", value.version }, { "auto_start", value.auto_start } };
    }

    inline void from_json(const nlohmann::json& json, LauncherSettings& value)
    {
        if (json.contains("version"))
            json.at("version").get_to(value.version);
        if (json.contains("auto_start"))
            json.at("auto_start").get_to(value.auto_start);
        value.version = LauncherSettings::CurrentVersion;
    }

    inline bool save_video_settings(const stdfs::path& path, VideoSettings value, std::string* error = nullptr)
    {
        value.validate();
        std::error_code ec;
        stdfs::create_directories(path.parent_path(), ec);
        if (ec) {
            if (error)
                *error = ec.message();
            return false;
        }

        const auto temporary = path.string() + ".tmp";
        {
            std::ofstream output{ temporary, std::ios::trunc };
            if (!output) {
                if (error)
                    *error = "failed to open temporary settings file";
                return false;
            }
            output << nlohmann::json(value).dump(4) << '\n';
            if (!output) {
                if (error)
                    *error = "failed to write temporary settings file";
                return false;
            }
        }

        stdfs::rename(temporary, path, ec);
        if (ec) {
            stdfs::remove(path, ec);
            ec.clear();
            stdfs::rename(temporary, path, ec);
        }
        if (ec && error)
            *error = ec.message();
        return !ec;
    }

    inline VideoSettings load_video_settings(const stdfs::path& path, const VideoSettings& defaults,
                                             bool* recovered = nullptr, std::string* error = nullptr)
    {
        try {
            std::ifstream input{ path };
            if (!input)
                throw std::runtime_error("settings file does not exist");
            auto value = nlohmann::json::parse(input).get<VideoSettings>();
            if (recovered)
                *recovered = false;
            return value;
        }
        catch (const std::exception& ex) {
            if (error)
                *error = ex.what();
            if (recovered)
                *recovered = true;
            save_video_settings(path, defaults);
            return defaults;
        }
    }

    inline bool save_launcher_settings(const stdfs::path& path, const LauncherSettings& value,
                                       std::string* error = nullptr)
    {
        std::error_code ec;
        stdfs::create_directories(path.parent_path(), ec);
        if (ec) {
            if (error)
                *error = ec.message();
            return false;
        }

        const auto temporary = path.string() + ".tmp";
        {
            std::ofstream output{ temporary, std::ios::trunc };
            if (!output) {
                if (error)
                    *error = "failed to open temporary launcher settings file";
                return false;
            }
            output << nlohmann::json(value).dump(4) << '\n';
        }

        stdfs::rename(temporary, path, ec);
        if (ec) {
            stdfs::remove(path, ec);
            ec.clear();
            stdfs::rename(temporary, path, ec);
        }
        if (ec && error)
            *error = ec.message();
        return !ec;
    }

    inline LauncherSettings load_launcher_settings(const stdfs::path& path,
                                                    const LauncherSettings& defaults = {},
                                                    std::string* error = nullptr)
    {
        try {
            std::ifstream input{ path };
            if (!input)
                throw std::runtime_error("launcher settings file does not exist");
            return nlohmann::json::parse(input).get<LauncherSettings>();
        }
        catch (const std::exception& ex) {
            if (error)
                *error = ex.what();
            save_launcher_settings(path, defaults);
            return defaults;
        }
    }
} // namespace codex::shipped
