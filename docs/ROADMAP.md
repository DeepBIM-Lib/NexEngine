# 路线图

路线图只保存粗粒度方向，不保存当前任务的详细实施步骤。

## 当前状态

Phase 0–6 已交付。引擎处于**功能可用、架构成型**状态：场景由脚本数据驱动，模块边界清晰，资源生命周期自动化。

下一步从 Phase 7 开始，按功能优先推进：先补齐场景表达能力（点线、透明、二维），再做大体量性能优化。

## 已完成

### Phase 0：最小引擎基础

- 自有数学定义与 DirectXMath 加速层。
- String、Array、Variant 等基础类型。
- Handle/HandleTable 句柄体系与 ResourceManager 统一生命周期（generation 防 ABA、fence 退役）。
- Transform、Movement、Input、Camera 模块基础（Position/Rotation/Scale、Kinematic 传播、FlyController）。

### Phase 1：ECS + 渲染 smoke

- Flecs C++ 模块化：Core / Input / Camera / Geometry / Material / Lighting / Renderer 各模块独立注册，边界清晰。
- 内建 Cube / QuadSphere 经 `(PrimitiveSource, Type)` Exclusive pair 按需生成，观察者驱动 MeshHandle 创建与替换。
- Texture、Mesh、Material 管理器与产品材质渲染 smoke。
- `minimal.nex` 脚本驱动的最小可见渲染路径（`MainScene` + `MainCamera` 实体契约）。

### Phase 2：原子化渲染 + 几何来源

- `Renderer{RenderType, Visible}` 组件统一拓扑与可见性表达。
- MeshManager 以 VertexPool / BufferSuballocator 分页子分配，超限自动建 dedicated page。
- `Scene` 槽位化（`MaxActiveScenes = 8`，业务层分配 index），实例提取按 ChildOf 向上归属场景。
- 相机即实体：`Camera` + `Perspective`/`Orthographic` + `RenderView` pair，多表面绑定与视口提取。

### Phase 3：Phong + PBR 引入

- `PBRMaterial` / `PhongMaterial` 模型组件 + `MaterialSource` pair + `Material` 表面状态（AlphaMode、DoubleSided、AlphaCutoff）。
- MaterialManager 按完整 descriptor 规范化复用资源，观察者驱动创建 / 替换 / 释放。
- MaterialBindingCache 的 SRB 缓存与表版本刷新；PipelineRegistry 的 PSO / technique 解析。

### Phase 4：现代化渲染管线

- FramePlan 编译管线（Clear / Forward / Present），主循环只执行已编译计划。
- `PostFrame` 统一提取：场景光照、RenderView 与实例，每帧从活 ECS 重建，无跨帧悬挂。
- 按 Mesh + Material 稳定排序合并 SubmissionBatch，radix 排序键内含场景 / 拓扑维度。
- 多 view 复用同一已编译计划，独立解析相机、RT/DS 与 resize。

### Phase 5：现代化材质与光照

- `EnvironmentLight` / `DirectionalLight` 挂载 Scene 实体，每场景独立光照，数据经消毒校验。
- Forward PBR / Phong shader 使用显式环境光、方向光、normal / ORM / emissive、alpha mask 与双面表面状态；零光照没有隐藏保底。
- PersistentBufferManager 持久槽 + dirty-bit 增量上传 + GPU 退役，transform / 材质常量槽位化。
- `minimal.nex` 显式声明环境光、方向光与 PBR / Phong source；完成多后端渲染验证。

### Phase 6：资源复用与 Batch/Instancing

- 相同 descriptor 的 Mesh / Material 全局规范化复用；persistent / built_in 资源防误删，teardown `ClearImmediate` 兜底。
- transform-only 共享 instance ABI 与按需倍增的 instance buffer，单 draw 多实例提交。
- flecs script 数据驱动创作就绪（template / prop / `@tree` / value pair）。
- 程序化大规模城市压力样例（`city.nex`）全链路验证；D3D11 / D3D12 / Vulkan smoke 与多视角、离屏输出验证。

## 已知边界（未接通或占位能力）

- **Point / Spot 光**：组件注册与 shader 循环就绪，提取与上传路径未接。
- **Line / Point 拓扑**：提取与批次构建在跑，Forward 仅绘制 TriangleList；topology overlay shader 无消费者。
- **ShadowMap**：`RenderPassType::ShadowMap` 枚举占位，无 technique。
- **DeferredEngine 与 Command / Response API**：占位空桩，仅 ImmediateEngine 驱动完整渲染循环。
- **多 view 共 surface 的 clear**：契约限定为组内 order 最小 view 决定整块 clear，per-view clear 未支持。
- **材质纹理运行时变更**：SRB 只按表版本刷新，纹理槽变更不重绑；MaterialBindingCache 无淘汰，随 world 退出清理。
- **MeshManager 页面回收**：页面只增不减，dedicated page 长期钉住，无碎片整理。
- **自动化回归**：开发阶段策略下暂缺。

## 后续规划

### 点线 Overlay 管线

- 接通 Line / Point 拓扑绘制与 topology overlay shader（提取与批次构建已在跑，补绘制路径）。
- 支撑 BIM 线框、标注类展示。

### 透明管线

- Blend / Transparency 绘制路径：透明对象与不透明场景分离排序、混合。
- 支持玻璃、水体等常见材质；与 Overlay 共享叠加合成路径。

### 二维支持（接入 Skia）

- 接入 Skia，提供二维 UI / 标注层，与三维场景混合渲染。
- 复用 Phase 7/8 的叠加合成路径。

### 大体量模型渲染（性能优化）

- CPU/GPU Culling：Frustum 剔除、可见集合、indirect args。
- 资源侧优化：MeshManager 页面回收与碎片整理。

## 其他候选

- 阴影与遮挡剔除（Depth Prepass、HiZ/Occlusion），在大体量性能阶段之后顺路推进。
- IBL 与环境贴图。
- Point / Spot 光接通（提取 → 上传 → 绘制）。
- per-view clear（scissored clear-quad，per-view 数据已就绪）。
- Shader 与 Material hot reload。