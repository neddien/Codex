#ifndef CODEX_H
#define CODEX_H

// Audio
#include <Engine/Audio/AudioManager.h>

// Core
#include <Engine/Core/Application.h>
#include <Engine/Core/Public/CommonDef.h>
#include <Engine/Core/Public/Exception.h>
#include <Engine/Core/Public/Geomtryd.h>
#include <Engine/Core/Public/Input.h>
#include <Engine/Core/Public/ResourceHandler.h>
#include <Engine/Core/Public/SerializationManager.h>
#include <Engine/Core/Public/Serializer.h>
#include <Engine/Core/Window.h>

// Debug
#include <Engine/Debug/Public/Profiler.h>
#include <Engine/Debug/Public/TimeScope.h>

// Concurrency
#include <Engine/Concurrency/Public/Mutex.h>

// Events
#include <Engine/Events/Event.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>

// File system
#include <Engine/FileSystem/Public/FileSystem.h>

// Math
#include <Engine/Math/Public/Math.h>

// Memory
#include <Engine/Memory/Public/Memory.h>

// Scene
#include <Engine/Scene/EditorCamera.h>
#include <Engine/Scene/Public/Camera.h>
#include <Engine/Scene/Public/Components.inl>
#include <Engine/Scene/Public/Entity.inl>
#include <Engine/Scene/Public/Scene.h>
#include <Engine/Scene/Public/SceneManager.h>

// Native Behaviour
#include <Engine/NativeBehaviour/Public/NativeBehaviourManager.h>

// Reflection
#include <Engine/Reflection/Public/Reflection.h>

// Graphics
#include <Engine/Graphics/BatchRenderer2D.h>
#include <Engine/Graphics/DebugDraw.h>
#include <Engine/Graphics/Public/Image2D.h>
#include <Engine/Graphics/Public/Shader.h>
#include <Engine/Graphics/Public/Texture2D.h>
#include <Engine/Graphics/RenderBatch.h>
#include <Engine/Graphics/Renderer.h>

// System
#include <Engine/System/DynamicLibrary.h>
#include <Engine/System/Process.h>

// Utils
#include <Engine/Utils/Public/Math.h>
#include <Engine/Utils/Public/Util.h>

// OpenGL
#include <Platform/OpenGL/FrameBuffer.h>
#include <Platform/OpenGL/Geometry.h>
#include <Platform/OpenGL/GraphicsCapabilities.h>
#include <Platform/OpenGL/IndexBuffer.h>
#include <Platform/OpenGL/Shader.h>
#include <Platform/OpenGL/Texture.h>
#include <Platform/OpenGL/VertexArray.h>
#include <Platform/OpenGL/VertexBuffer.h>
#include <Platform/OpenGL/VertexBufferLayout.h>

#endif // CODEX_H
