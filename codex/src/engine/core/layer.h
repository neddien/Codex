#ifndef CODEX_CORE_LAYER_H
#define CODEX_CORE_LAYER_H

#include <engine/events/event.h>

namespace codex {
    class CODEX_API Layer
    {
    protected:
        std::string debug_name_;

    public:
        Layer(const std::string_view name = "Layer");
        virtual ~Layer() = default;

    public:
        virtual void on_attach() {}
        virtual void on_detach() {}
        virtual void on_update([[maybe_unused]] const f32 deltaTime) {}
        virtual void on_imgui_render() {}
        virtual void on_event([[maybe_unused]] events::Event& event) {}

        const std::string& name() const noexcept { return debug_name_; }
    };
} // namespace codex

#endif // CODEX_CORE_LAYER_H
