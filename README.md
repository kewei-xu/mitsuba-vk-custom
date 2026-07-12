# Mitsuba 3 Gonioreflectometer

这是一个基于 Mitsuba 3.7.1 的双向反射分布函数（BRDF）/半球反射测量扩展。它把准直入射光从场景外发射到 OBJ 模型，通过 BSDF 追踪光线，并把离开场景的方向累积到等面积球面网格中。

仓库同时提供稳定实现和 GPU 优化实现。两者使用相同的网格、层定义和物理路径追踪逻辑，插件名称不同，因此可以并行安装并做结果对比。

## 1. 实现选择

| 用途 | 目录 | 插件名称 | 输出特点 |
| --- | --- | --- | --- |
| 结果验证、调试、标定 | `src/gonio` | `gonio`、`directionalsimple`、`gtracer` | Python 端直接导出 BMP 和 `.dat` |
| CUDA/Dr.Jit 批量计算与性能测试 | `src/gonio_gpu_opt` | `gonio_gpu_opt`、`directionalsimple_gpu_opt`、`gtracer_gpu_opt` | NumPy 加速解码，可导出 EXR，支持投影缓存和计时 |

GPU 版本当前已经在 `src/CMakeLists.txt` 中注册，并不会覆盖稳定版本。第一次恢复开发时建议先用 `src/gonio` 跑通一个小样例，再切换到 `_gpu_opt` 版本。

## 2. Windows + Visual Studio 2022 环境

假设仓库位于 `F:\post-doc\code\mitsuba-3.7.1-gonio\mitsuba-vk-custom`，并且已经用 VS 2022 编译完成。

在 PowerShell 中先加载构建目录的运行环境：

```powershell
cd F:\post-doc\code\mitsuba-3.7.1-gonio\mitsuba-vk-custom
. .\build\setpath.ps1
$env:PYTHONPATH = (Resolve-Path .\src).Path + ";" + $env:PYTHONPATH
```

`build\setpath.ps1` 会设置 `MITSUBA_DIR`、DLL 搜索路径和 `build\python`。追加 `src` 是为了让 Python 找到 `gonio.python` 与 `gonio_gpu_opt.python`。

如果需要重新配置：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target ALL_BUILD
```

也可以直接打开 `build\mitsuba.sln`，构建 `ALL_BUILD`。插件 DLL 默认位于 `build\Debug\plugins` 或对应配置目录。

## 3. 最小运行例子

仓库内有一个可用于烟雾测试的网格：`src/gonio/tests/resources/quad.obj`。稳定实现的命令如下：

```powershell
python .\src\gonio\python\gonio_export.py `
  --obj .\src\gonio\tests\resources\quad.obj `
  --output .\runs\demo_scalar `
  --variant scalar_rgb `
  --samples 4096 `
  --precision 1 `
  --theta-i 30 `
  --phi-i 0 `
  --bsdf diffuse `
  --normalize surface
```

GPU 优化实现的命令如下。`cuda_rgb` 需要可用的 NVIDIA CUDA/OptiX 运行环境；如果只想先验证接口，可将 `--variant` 改成已编译的其他 JIT 变体。

```powershell
python .\src\gonio_gpu_opt\python\gonio_export.py `
  --obj .\src\gonio_gpu_opt\tests\resources\quad.obj `
  --output .\runs\demo_gpu `
  --variant cuda_rgb `
  --samples 100000 `
  --samples-per-pass 262144 `
  --precision 2 `
  --theta-i 30 `
  --phi-i 0 `
  --include-projection `
  --bsdf conductor `
  --material Au `
  --normalize surface
```

批量扫描入射角：

```powershell
python .\src\gonio\python\run_gonio_batch.py `
  --obj .\src\gonio\tests\resources\quad.obj `
  --output-root .\runs\theta_sweep `
  --variants scalar_rgb `
  --thetas 0 30 45 80 `
  --samples 4096 `
  --precision 1
```

GPU 版本有相同功能的脚本：

```powershell
python .\src\gonio_gpu_opt\python\run_gonio_batch.py `
  --obj .\src\gonio_gpu_opt\tests\resources\quad.obj `
  --output-root .\runs\theta_sweep_gpu `
  --variants cuda_rgb `
  --thetas 0 30 45 80 `
  --samples 100000 `
  --precision 2
```

## 4. Mitsuba 场景接口

如果不使用 Python 导出器，也可以在 Mitsuba 场景字典或 XML 中直接使用这些插件：

```json
{
  "integrator": {
    "type": "gtracer",
    "max_depth": 8,
    "rr_depth": 8
  },
  "sensor": {
    "type": "gonio",
    "precision": 1,
    "merge_l1_l2": false,
    "record_refraction": false,
    "sampler": {"type": "independent", "sample_count": 4096},
    "film": {"type": "hdrfilm", "width": 1, "height": 1}
  },
  "emitter": {
    "type": "directionalsimple",
    "direction": [0, 0, -1],
    "irradiance": {"type": "rgb", "value": [1, 1, 1]}
  }
}
```

优化版本只需把三个类型替换为 `gtracer_gpu_opt`、`gonio_gpu_opt` 和 `directionalsimple_gpu_opt`。`directionalsimple` 的 `direction` 是光线传播方向，不是从物体指向光源的反向方向。

## 5. 重要参数

- `precision`：等面积球面网格精度，只支持 `1`–`4`。对应 patch 数为 `1663`、`26279`、`53224`、`523910`。精度越高，内存和解码时间越大。
- `theta-i`、`phi-i`：入射方向，单位是度；当前 Python 接口要求 `0 <= theta-i < 90`、`0 <= phi-i < 360`。
- `merge-l1-l2`：把一次反射和多次反射合并到同一个 L1 输出层。默认关闭。
- `record-refraction`：同时保留上、下半球；开启后会生成 `H+` 和 `H-` 层。
- `max-depth`：最大路径深度，`-1` 表示不限制。
- `rr-depth`：从第几次反弹开始 Russian roulette；应为正数。
- `normalize`：`surface` 按命中表面的样本数归一化，`sensor` 按落入测量网格的样本数归一化，`none` 不归一化。比较不同入射角时通常使用 `surface`。
- GPU 版本的 `samples-per-pass` 控制每个 wavefront 的大小；显存不足时应减小它。
- GPU 版本的 `--include-projection` 会额外生成二维半球投影；不需要预览时关闭可减少 CPU 解码开销。

## 6. 输出文件

每个输出目录包含 `render_config.json`、`sensor_summary.json` 和 `cooked_sensor_valid_sample_count.txt`。`imgs` 目录中的主要文件为：

- `sensor_L1_H+_sensorData.dat` 等：按 patch 顺序排列的 RGB 浮点数据。
- `sensor_H+_sensorTheta.dat`、`sensor_H+_sensorPhi.dat`：每个 patch 的代表角度，单位是弧度。
- `sensor_L1_H+` 等无扩展名文本：RGB 总能量。
- 稳定版的 `raw_sensor.bmp` 和层 BMP：便于快速查看，但经过了归一化和 gamma 映射，仅用于预览。
- GPU 版的 `raw_sensor.exr` 和可选层 EXR：保留高动态范围，适合后处理。

`.dat` 是连续的 32-bit 浮点二进制数据；Windows 版本使用小端序。每个 RGB 文件的逻辑形状为 `(patch_count, 3)`，角度文件的逻辑形状为 `(patch_count,)`。

## 7. 开发入口

推荐按以下顺序阅读和修改：

1. `src/gonio/common/gonio_grid.h`：网格构造和方向到 patch 的索引规则。
2. `src/gonio/sensors/gonio_sensor.h`：film 宽高、header、L1/L2、H+/H- 的布局。
3. `src/gonio/integrators/gtracer.cpp`：发射、BSDF 采样、路径追踪和结果累积。
4. `src/gonio/emitters/directionalsimple.cpp`：准直光源的 AABB 发射策略。
5. `src/gonio/python/gonio_export.py`：raw film 解码、归一化和文件输出。
6. `src/gonio_gpu_opt/python/gonio_export.py`：NumPy、缓存、EXR 和计时优化。

完整的迁移背景、当前状态和后续工作记录见 [DEVELOPMENT_LOG.md](DEVELOPMENT_LOG.md)。原始 Mitsuba 3 的通用文档见 [官方文档](https://mitsuba.readthedocs.io/)。

## 8. 许可证与引用

本仓库保留 Mitsuba 3 的许可证和第三方依赖许可证。发表结果时请同时按照项目要求引用 Mitsuba 3、Dr.Jit 以及你所采用的 gonioreflectometer 数据/算法来源。
