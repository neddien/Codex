#pragma once

// Audio
#include <engine/audio/audio_manager.h>

// Core
#include <engine/core/engine.h>
#include <engine/core/public/common_def.h>
#include <engine/core/public/exception.h>
#include <engine/core/public/geometry.h>
#include <engine/core/public/input.h>
#include <engine/core/public/resource_handler.h>
#include <engine/core/public/serialization_manager.h>
#include <engine/core/public/serializer.h>
#include <engine/core/window.h>

// Debug
#include <engine/debug/public/profiler.h>
#include <engine/debug/public/time_scope.h>

// Concurrency
#include <engine/concurrency/public/mutex.h>
#include <engine/concurrency/public/task.h>

// Events
#include <engine/events/event.h>
#include <engine/events/key_event.h>
#include <engine/events/mouse_event.h>

// File system
#include <engine/file_system/cxpak.h>
#include <engine/file_system/disk_mount.h>
#include <engine/file_system/ivfs_mount.h>
#include <engine/file_system/memory_mount.h>
#include <engine/file_system/pak_mount.h>
#include <engine/file_system/public/file_system.h>
#include <engine/file_system/vfs.h>

// Math
#include <engine/math/public/math.h>

// Memory
#include <engine/memory/public/memory.h>

// Scene
#include <engine/scene/editor_camera.h>
#include <engine/scene/public/camera.h>
#include <engine/scene/public/components.inl>
#include <engine/scene/public/entity.inl>
#include <engine/scene/public/scene.h>
#include <engine/scene/public/scene_manager.h>

// Native Behaviour
#include <engine/native_behaviour/public/native_behaviour_manager.h>

// Reflection
#include <engine/reflection/public/reflection.h>

// Graphics
#include <engine/graphics/batch_renderer2d.h>
#include <engine/graphics/debug_draw.h>
#include <engine/graphics/public/image2d.h>
#include <engine/graphics/public/shader.h>
#include <engine/graphics/public/texture2d.h>
#include <engine/graphics/render_batch.h>
#include <engine/graphics/renderer.h>

// System
#include <engine/system/dynamic_library.h>
#include <engine/system/process.h>

// Utils
#include <engine/utils/public/math.h>
#include <engine/utils/public/util.h>

// OpenGL
#include <platform/open_gl/frame_buffer.h>
#include <platform/open_gl/geometry.h>
#include <platform/open_gl/graphics_capabilities.h>
#include <platform/open_gl/index_buffer.h>
#include <platform/open_gl/shader.h>
#include <platform/open_gl/texture.h>
#include <platform/open_gl/vertex_array.h>
#include <platform/open_gl/vertex_buffer.h>
#include <platform/open_gl/vertex_buffer_layout.h>
