# NexEngine

A real-time rendering engine built with C++20 (C++17 compatibility for externals), Flecs ECS, and Diligent Engine.

Supports D3D11, D3D12, and Vulkan on Windows.

## Agent Documentation

AI agents must consult the following documents before starting any task:

| Document | Purpose |
|---|---|
| [AGENTS.md](AGENTS.md) | Agent reading order, change gates, and delivery protocol |
| [docs/PROJECT.md](docs/PROJECT.md) | Project positioning, architectural invariants, and fact boundaries |
| [docs/ENGINEERING.md](docs/ENGINEERING.md) | Engineering rules, code style, and API constraints |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Completed milestones and planned directions |

## Building Locally

```powershell
git submodule update --init --recursive
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build --config Debug --target GlfwViewer
```