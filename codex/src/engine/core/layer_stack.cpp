#include "layer_stack.h"

namespace codex {
    LayerStack::~LayerStack()
    {
        clear();
    }

    void LayerStack::clear() noexcept
    {
        for (auto* layer : layers_) {
            layer->on_detach();
            delete layer;
        }
        layers_.clear();
        layer_pointer_ = 0;
    }

    void LayerStack::push_layer(Layer* layer)
    {
        layers_.insert(layers_.begin() + layer_pointer_++, layer);
        // layers_.emplace(layers_.begin() + layer_pointer_++, layer);
    }

    void LayerStack::push_overlay(Layer* overlay)
    {
        layers_.push_back(overlay);
    }

    void LayerStack::pop_layer(Layer* layer)
    {
        auto it = std::find(layers_.begin(), layers_.begin() + layer_pointer_, layer);
        if (it != layers_.begin() + layer_pointer_) {
            layer->on_detach();
            layers_.erase(it);
            --layer_pointer_;
        }
    }

    void LayerStack::pop_overlay(Layer* overlay)
    {
        auto it = std::find(layers_.begin(), layers_.begin() + layer_pointer_, overlay);
        if (it != layers_.end()) {
            overlay->on_detach();
            layers_.erase(it);
        }
    }
} // namespace codex
