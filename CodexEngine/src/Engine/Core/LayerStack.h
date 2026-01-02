#ifndef CODEX_CORE_LAYER_STACK_H
#define CODEX_CORE_LAYER_STACK_H

#include <sdafx.h>

#include "Layer.h"

namespace codex {
    class CODEX_API LayerStack
    {
    private:
        std::vector<Layer*> m_Layers;
        u32                 m_LayerPointer = 0;

    public:
        LayerStack() = default;
        ~LayerStack();

    public:
        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

    public:
        auto begin() noexcept { return m_Layers.begin(); }
        auto end() noexcept { return m_Layers.end(); }
        auto rbegin() noexcept { return m_Layers.rbegin(); }
        auto rend() noexcept { return m_Layers.rend(); }
        auto begin() const noexcept { return m_Layers.begin(); }
        auto end() const noexcept { return m_Layers.end(); }
        auto rbegin() const noexcept { return m_Layers.rbegin(); }
        auto rend() const noexcept { return m_Layers.rend(); }
    };
} // namespace codex

#endif // CODEX_CORE_LAYER_STACK_H
