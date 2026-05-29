#pragma once

#include <imgui.h>

#include <engine/core/engine.h>

extern codex::Engine* codex::create_engine(EngineArgs args);

class ImGuiContext;
extern ImGuiContext* GImGui;

int main(int argc, char** argv)
{
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
