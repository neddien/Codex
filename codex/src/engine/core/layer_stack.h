#ifndef CODEX_CORE_LAYER_STACK_H
#define CODEX_CORE_LAYER_STACK_H

#include "layer.h"

namespace codex {
    class CODEX_API LayerStack
    {
    private:
        std::vector<Layer*> layers_;
        u32                 layer_pointer_ = 0;

    public:
        LayerStack() = default;
        ~LayerStack();

    public:
        void push_layer(Layer* layer);
        void push_overlay(Layer* overlay);
        void pop_layer(Layer* layer);
        void pop_overlay(Layer* overlay);
        void clear() noexcept { layers_.clear(); }

    public:
        auto begin() noexcept { return layers_.begin(); }
        auto end() noexcept { return layers_.end(); }
        auto rbegin() noexcept { return layers_.rbegin(); }
        auto rend() noexcept { return layers_.rend(); }
        auto begin() const noexcept { return layers_.begin(); }
        auto end() const noexcept { return layers_.end(); }
        auto rbegin() const noexcept { return layers_.rbegin(); }
        auto rend() const noexcept { return layers_.rend(); }
    };
} // namespace codex

#endif // CODEX_CORE_LAYER_STACK_H
