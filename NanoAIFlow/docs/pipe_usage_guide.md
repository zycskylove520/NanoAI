# NanoAI 管线框架用法指南（当前版本）

> 本文档对应当前 `core/pipe.hpp` 与 `core/pipeline.hpp` 的实现。
> 重点：NanoAI 现为**静态类型、编译期固定节点**的管线模型。

## 1. 核心概念

- `NanoPipe<Derived, MaxConcurrency, Policy, DedicatedPoolSize>`
  - Pipe 的 CRTP 基类。
  - 你只需实现 `on_run(...)` 业务逻辑。
- `NanoPipeLine<P1, P2, ...>`
  - 编译期固定节点序列。
  - `run(...)` 时自动按阶段调用。
- `PipeExecutionPolicy`
  - `shared_pool`：使用管线共享线程池。
  - `dedicated_pool`：为该阶段使用独立线程池。
  - `inline_run`：调用线程内直接执行。

## 2. 定义一个 Pipe

```cpp
#include "core/pipe.hpp"

class AddOnePipe : public NanoAI_FLOW::NanoPipe<AddOnePipe>
{
public:
    int on_run(int x)
    {
        return x + 1;
    }
};
```

说明：
- `on_run` 的参数类型和返回类型就是该阶段的输入输出契约。
- 框架会自动调用 `run(...) -> on_run(...)`。

## 3. 构建并运行静态管线

### 3.1 直接构造（推荐用于节点固定场景）

```cpp
#include "core/pipeline.hpp"

class MulPipe : public NanoAI_FLOW::NanoPipe<MulPipe>
{
public:
    int on_run(int x)
    {
        return x * 2;
    }
};

AddOnePipe add;
MulPipe mul;

auto pipeline = NanoAI_FLOW::make_pipeline<NanoAI_FLOW::PipeForwardOrder::ordered>(
    8,
    64,
    add,
    mul);
int out = pipeline.run(10);  // (10 + 1) * 2 = 22
```

### 3.2 使用 builder 链式追加节点

```cpp
auto pipeline = NanoAI_FLOW::make_pipeline_builder<NanoAI_FLOW::PipeForwardOrder::ordered>(8, 64)
                    .add_pipe(AddOnePipe{})
                    .add_pipe(MulPipe{})
                    .build();

int out = pipeline.run(10);
```

注意：
- builder 仅支持**右值链式**（临时对象链式调用）。
- 左值 builder 的 `add_pipe(...)` 与 `build()` 已禁用，误用会在编译期报错。
- 正确写法：`make_pipeline_builder<...>(...).add_pipe(...).build()`。

## 4. 参数传递规则

- 单参数：按 `on_run` 签名直接传递。
- 多参数：上游返回 `std::tuple<...>` 时，下游会自动解包为多参数调用。

示例：

```cpp
class PackPipe : public NanoAI_FLOW::NanoPipe<PackPipe>
{
public:
    std::tuple<int, std::string> on_run(int x)
    {
        return {x, "ok"};
    }
};

class ConsumePipe : public NanoAI_FLOW::NanoPipe<ConsumePipe>
{
public:
    std::string on_run(int x, const std::string& tag)
    {
        return tag + ":" + std::to_string(x);
    }
};
```

## 5. 并发与执行策略

```cpp
class SharedPipe : public NanoAI_FLOW::NanoPipe<
    SharedPipe,
    4,
    NanoAI_FLOW::PipeExecutionPolicy::shared_pool>
{
public:
    int on_run(int x) { return x + 1; }
};

class DedicatedPipe : public NanoAI_FLOW::NanoPipe<
    DedicatedPipe,
    4,
    NanoAI_FLOW::PipeExecutionPolicy::dedicated_pool,
    2>
{
public:
    int on_run(int x) { return x * 2; }
};
```

说明：
- `MaxConcurrency` 控制阶段最大并发。
- `dedicated_pool` 下可通过 `DedicatedPoolSize` 指定独立池大小。
- `global_task_quota` 控制整个管线并发任务上限。

## 6. 常见误区

1. 误区：
    - `auto b = make_pipeline_builder<...>(...); b.add_pipe(a);`
    - `auto b = make_pipeline_builder<...>(...); b.build();`
    - 问题：左值 builder 已禁用这两个接口，会直接编译报错。
    - 建议：改为右值链式：`make_pipeline_builder<...>(...).add_pipe(...).build()`。

2. 误区：继续使用 `PipeValue/NanoPipeBase/std::any_cast`。
   - 问题：这是旧接口模型。
   - 建议：改为 `on_run(强类型参数)` + `强类型返回`。

3. 误区：运行时频繁改管线结构。
   - 说明：当前模型强调编译期固定结构，适合稳定数据流。

## 7. 参考文件

- `core/pipe.hpp`
- `core/pipeline.hpp`
- `examples/pipeline_quickstart.cpp`
