# Gonioreflectometer 开发日志

本文档用于恢复开发上下文。它记录的是当前 Git 分支 `gonioreflectometer` 中已经存在的迁移结果，而不是一份新的算法设计。

## 基线与版本

- 基础工程：Mitsuba 3.7.1 风格的源码树。
- 当前分支：`gonioreflectometer`。
- 迁移提交：`4e062cd9`，提交说明为 `first version improved by Codex`。
- 迁移前父提交：`93e53079`，对应仓库的稳定基线。
- 本地构建方式：Visual Studio 2022 + CMake；当前构建目录已经生成了稳定插件和 `_gpu_opt` 插件的 Debug DLL。

## 迁移目标

原始 gonioreflectometer 逻辑被拆成 Mitsuba 插件，而不是直接修改 Mitsuba 的核心 `render/` 头文件。这样做的好处是：

- 测量传感器、准直光源和测量积分器可以独立编译和替换。
- 稳定实现与 GPU 优化实现可以同时存在，用不同插件名做回归比较。
- Python 解码和导出逻辑不必混入 Mitsuba 的 C++ 核心。

## 已完成的模块

### 1. 等面积半球网格

`common/gonio_grid.h` 和两个 Python 导出器共同实现了同一套 ring 网格。精度 `1`–`4` 映射到固定的 patch 数：

| precision | patch 数 | cap 角度 |
| ---: | ---: | ---: |
| 1 | 1,663 | 1.987071° |
| 2 | 26,279 | 0.4998442° |
| 3 | 53,224 | 0.3512243° |
| 4 | 523,910 | 0.1119462° |

网格先建立中心 spherical cap，再按 stereographic radius 估计后续 ring 的 patch 数，并用球面面积方程修正 ring 边界。最后通过 `k_p == patch_count` 做内部一致性检查。这个检查很重要，修改网格公式后应优先确认它仍然通过。

### 2. `GonioSensor`

传感器不是相机，不会生成 primary ray；它只是一个测量累积器。它负责：

- 把 `(phi, theta)` 映射到 patch。
- 在 JIT 变体中把 ring 元数据上传到 `DynamicBuffer`，用 binary search + gather 完成索引。
- 决定 film 层布局。
- 在 `record_refraction` 开启时把下半球折叠到相同网格，并用 `H-` 层保留半球信息。

非 analytic 模式的 film 第一个 x 单元是 header，用来记录命中场景表面的样本数；实际 angular patch 从 x=1 开始。这个约定是 Python 解码器和 C++ sensor 之间最容易出错的接口。

### 3. `DirectionalSimpleEmitter`

这是一个无限、delta-direction 的准直光源。它在场景 AABB 的上游面采样起点，沿固定方向发射平行光线。`direction` 表示光线传播方向，因此与“从样品指向光源”的方向相反。

### 4. `GonioTracerIntegrator`

积分器执行 light tracing：

```text
DirectionalSimpleEmitter
        -> 初始光线
        -> 场景求交
        -> BSDF 采样和 throughput 更新
        -> Russian roulette / max_depth
        -> 逃逸方向转为 phi/theta
        -> GonioSensor film 的 patch/layer
```

几何法线修正、折射率 `eta`、Russian roulette 和 Dr.Jit loop state 都位于 `integrators/gtracer.cpp`。GPU 优化版另外把每个样本的表面命中数返回给 integrator，最后一次性写入 header，避免在 wavefront 中反复写同一个计数位置。

### 5. Python 导出器

`gonio_export.py` 负责场景字典构造、调用 `integrator.render()`、读取 raw film、跳过 header、按 layer 解码、归一化和写文件。

稳定版以简单和可读性为主，输出 BMP 预览。GPU 版额外做了：

- NumPy 可选加速；没有 NumPy 时仍使用 Python `array` 回退路径。
- patch center、projection index 和 source offset 的内存缓存。
- raw film 与可选投影使用 EXR，保留高动态范围。
- `load_dict`、render、decode、projection 等阶段计时。
- `samples_per_pass`、`coalesce` 和可替换插件名称参数。

## 当前代码组织

```text
src/gonio/                 稳定实现
src/gonio_gpu_opt/         GPU 优化实现
  common/                  C++ 网格和 JIT buffer
  sensors/                 测量 sensor
  emitters/                准直 emitter
  integrators/             light-tracing integrator
  python/                  渲染、解码、批处理、profiling
  tests/                   Python 回归测试和小型 OBJ
```

`src/gonio_gpu_opt/EXPERIMENTAL.md` 以前写着“尚未接入构建”，这是旧状态。本次检查确认它已经在 `src/CMakeLists.txt` 中注册，所以该说明已经更新；它仍然属于实验实现，不能自动视为与稳定版完全等价。

## 恢复开发时的推荐顺序

1. 使用 `scalar_rgb`、`precision=1` 和几千样本跑通最小 OBJ。
2. 检查 `sensor_summary.json`、`surface_hits` 和 `sensor_L1_H+_sensorData.dat` 是否有非零值。
3. 用同一场景、同一 seed 运行稳定版和 `_gpu_opt` 版，先比较 patch/layer 布局，再比较数值。
4. 再提高 `precision` 和样本数；precision 4 会显著增加 film 宽度和导出内存。
5. 最后开启 `record_refraction`、`merge_l1_l2`、多 pass 和 projection，逐项确认输出文件。

## 已知注意事项

- `src/gonio` 与 `src/gonio_gpu_opt` 中存在有意的镜像代码；修改网格、层布局或方向约定时必须同步修改两处和对应 Python 解码器。
- Python 的 `analytic_measurement` 选项目前主要由 C++ sensor 支持，默认导出流程使用普通 header 布局；扩展 analytic 流程时要同步调整 `header_cells`、layer descriptors 和 raw film 解码。
- `src/gonio` 与优化版输出格式不同：稳定版预览是 BMP，优化版 raw film 是 EXR。不要用预览 BMP 做定量比较。
- 仓库历史提交包含较多 `runs/` 结果文件。后续开发若新增大规模实验，建议把可复现配置和摘要纳入版本控制，避免把全部二进制输出继续提交进源码历史。
- 当前没有在本次整理中改变传输算法或输出数值，只添加了注释、文档和过时状态说明的修正。

## 后续工作建议

- 为两套实现建立明确的 scalar-vs-GPU 数值容差回归测试。
- 将共享的 Python 网格/层描述抽成一个公共模块，避免双份实现继续漂移。
- 给 `analytic_measurement` 增加端到端导出测试。
- 把批量输出目录、缓存目录和源码目录在 Git 中进一步分离。
- 在 CUDA/OptiX 与 Windows 构建矩阵中记录固定场景、seed、samples-per-pass 和耗时。
