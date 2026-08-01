#include <codex.h>
#include <engine/core/entry_point.h>

#include <imgui.h>

#include <launcher_settings.h>

using namespace codex;

namespace {
    namespace stdfs = std::filesystem;

    stdfs::path package_root(const EngineArgs& args)
    {
        if (args.count > 0) {
            std::error_code ec;
            auto            executable = stdfs::weakly_canonical(stdfs::absolute(args.args[0]), ec);
            if (!ec)
                return executable.parent_path().parent_path();
        }
        return stdfs::current_path();
    }

    class LauncherLayer final : public Layer
    {
    public:
        LauncherLayer(stdfs::path root, std::function<void(sys::Process::ProcessHandle)> on_runtime_launched)
            : root_{ std::move(root) }
            , settings_path_{ root_ / "config/video.json" }
            , launcher_settings_path_{ root_ / "config/launcher.json" }
            , on_runtime_launched_{ std::move(on_runtime_launched) }
        {
            settings_          = shipped::load_video_settings(settings_path_, {}, nullptr, &status_);
            launcher_settings_ = shipped::load_launcher_settings(launcher_settings_path_);
            if (!status_.empty())
                status_ = "Using default settings: " + status_;
        }

        void on_attach() override
        {
            if (launcher_settings_.auto_start)
                launch();
        }

        void on_imgui_render() override
        {
            ImGui::SetNextWindowPos({ 0, 0 });
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::Begin("Codex Launcher", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

            constexpr float content_width  = 520.0f;
            constexpr float content_height = 500.0f;
            const auto      available      = ImGui::GetContentRegionAvail();
            const float     left_margin    = std::max((available.x - content_width) * 0.5f, 0.0f);
            const float     top_margin     = std::max((available.y - content_height) * 0.5f, 0.0f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + left_margin);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + top_margin);
            ImGui::BeginChild("##content", { content_width, content_height }, false,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            const float aligned_width = ImGui::GetContentRegionAvail().x;
            ImGui::BeginChild("##banner", { aligned_width, 150.0f }, true,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            const char* banner_text = "Banner / Thumbnail";
            const auto  banner_size = ImGui::CalcTextSize(banner_text);
            ImGui::SetCursorPos(
                { (content_width - banner_size.x) * 0.5f, (ImGui::GetWindowHeight() - banner_size.y) * 0.5f });
            ImGui::TextDisabled("%s", banner_text);
            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::TextUnformatted("Video settings");
            ImGui::Separator();
            ImGui::Spacing();

            bool changed = false;
            if (ImGui::BeginTable("##video_settings", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed, 190.0f);
                ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

                const auto begin_setting = [](const char* label)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(label);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                };

                constexpr const char* modes[] = { "windowed", "borderless", "fullscreen" };
                int mode = settings_.display_mode == "fullscreen" ? 2 : settings_.display_mode == "borderless" ? 1 : 0;
                begin_setting("Display mode");
                if (ImGui::Combo("##display_mode", &mode, modes, 3)) {
                    settings_.display_mode = modes[mode];
                    changed                = true;
                }

                begin_setting("Width");
                changed |= ImGui::InputInt("##width", &settings_.width, 0, 0);
                begin_setting("Height");
                changed |= ImGui::InputInt("##height", &settings_.height, 0, 0);
                begin_setting("Monitor");
                changed |= ImGui::InputInt("##monitor", &settings_.monitor, 0, 0);
                begin_setting("Refresh rate");
                changed |= ImGui::InputInt("##refresh_rate", &settings_.refresh_rate, 0, 0);

                int frame_cap = static_cast<int>(settings_.frame_cap);
                begin_setting("Frame cap (0 = unlimited)");
                if (ImGui::InputInt("##frame_cap", &frame_cap, 0, 0)) {
                    settings_.frame_cap = static_cast<u32>(std::max(frame_cap, 0));
                    changed             = true;
                }

                begin_setting("VSync");
                changed |= ImGui::Checkbox("##vsync", &settings_.vsync);
                begin_setting("Resizable");
                changed |= ImGui::Checkbox("##resizable", &settings_.resizable);
                begin_setting("Auto-start game");
                if (ImGui::Checkbox("##auto_start", &launcher_settings_.auto_start))
                    save_launcher_settings();
                ImGui::EndTable();
            }

            if (changed)
                save();

            ImGui::Spacing();
            if (ImGui::Button("Launch", { ImGui::GetContentRegionAvail().x, 38.0f }))
                launch();

            if (!status_.empty())
                ImGui::TextWrapped("%s", status_.c_str());
            ImGui::EndChild();
            ImGui::End();
        }

    private:
        bool save()
        {
            status_.clear();
            if (!shipped::save_video_settings(settings_path_, settings_, &status_)) {
                status_ = "Failed to save video settings: " + status_;
                return false;
            }
            status_ = "Settings saved.";
            return true;
        }

        void launch()
        {
#ifdef CX_PLATFORM_WINDOWS
            const auto                 runtime         = root_ / "bin/ShippedRuntime.exe";
            constexpr std::string_view runtime_command = "bin\\ShippedRuntime.exe";
#else
            const auto                 runtime         = root_ / "bin/ShippedRuntime";
            constexpr std::string_view runtime_command = "./bin/ShippedRuntime";
#endif
            if (!stdfs::exists(runtime)) {
                status_ = "Runtime not found: " + runtime.string();
                return;
            }

            sys::ProcessInfo info;
            info.command         = runtime_command;
            info.cwd             = root_.string();
            info.system_shell    = false;
            auto runtime_process = sys::Process::create(std::move(info));
            try {
                runtime_process->launch();
                on_runtime_launched_(std::move(runtime_process));
            }
            catch (const std::exception& ex) {
                status_ = std::string{ "Failed to launch runtime: " } + ex.what();
            }
        }

        bool save_launcher_settings()
        {
            status_.clear();
            if (!shipped::save_launcher_settings(launcher_settings_path_, launcher_settings_, &status_)) {
                status_ = "Failed to save launcher settings: " + status_;
                return false;
            }
            return true;
        }

        stdfs::path                                      root_;
        stdfs::path                                      settings_path_;
        stdfs::path                                      launcher_settings_path_;
        shipped::VideoSettings                           settings_;
        shipped::LauncherSettings                        launcher_settings_;
        std::string                                      status_;
        std::function<void(sys::Process::ProcessHandle)> on_runtime_launched_;
    };

    class Launcher final : public Engine
    {
    public:
        Launcher(EngineProperties properties, stdfs::path root)
            : Engine{ std::move(properties) }
            , root_{ std::move(root) }
        {
        }

        ~Launcher() override
        {
            dispose();
            if (runtime_process_)
                runtime_process_->wait_for_exit();
        }

        void on_init() override
        {
            push_layer(new LauncherLayer(root_,
                                         [this](sys::Process::ProcessHandle process)
                                         {
                                             runtime_process_ = std::move(process);
                                             Engine::stop();
                                         }));
        }

    private:
        stdfs::path                 root_;
        sys::Process::ProcessHandle runtime_process_;
    };
} // namespace

Engine* codex::create_engine(EngineArgs args)
{
    auto root = package_root(args);
    EngineProperties properties{
        .name = "CodexLauncher",
        .cwd  = root,
        .args = std::move(args),
        .flags = EngineFlags::Input | EngineFlags::Video | EngineFlags::Logger,
        .video_properties = {
            .window_title  = "Codex Launcher",
            .window_width  = 540,
            .window_height = 460,
            .frame_cap     = 60,
            .window_flags  = WindowFlags::Visible | WindowFlags::PositionCentre | WindowFlags::Resizable,
            .vsync         = true,
        },
    };
    return new Launcher{ std::move(properties), std::move(root) };
}
