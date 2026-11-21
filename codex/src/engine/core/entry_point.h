#pragma once

#include <imgui.h>

#include <engine/core/engine.h>

extern codex::Engine* codex::create_engine(EngineArgs args);

class ImGuiContext;
extern ImGuiContext* GImGui;

void enable_console_colors()
{
#ifdef CX_PLATFORM_WINDOWS
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
        return;

    DWORD mode = 0;

    if (!GetConsoleMode(handle, &mode))
        return;

    SetConsoleMode(
        handle,
        mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING
    );
#endif
}

int main(int argc, char** argv)
{
    enable_console_colors();

    try {
        auto app = codex::create_engine({ argc, argv });
        GImGui   = codex::Engine::imgui_layer()->imgui_context();
        app->on_init();
        app->run();
        delete app;
    }
    catch (const codex::CodexException& ex) {
        codex::fatal(ex.to_string());
        std::exit(EXIT_FAILURE);
    }
    return 0;
}
