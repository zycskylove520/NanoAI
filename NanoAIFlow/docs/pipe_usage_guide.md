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

补充说明：
- 当前共享/专用线程运行时由 `core/thread_pool.hpp` 提供，基于 C++20 标准并发原语实现。
- 运行时不依赖第三方线程池头文件或外部线程库。

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
- 当前版本的共享/专用线程池都支持 move-only 任务封装，适合流水线内部异步链式调度。

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

## 7. 管线对象的值语义

- 当前版本 `NanoPipeLine` 支持拷贝构造与移动构造。
- 拷贝/移动后会创建独立的运行时调度状态（包括共享线程池与阶段状态），不会与原对象共享任务队列。

示例：

```cpp
auto p = make_pipeline<PipeForwardOrder::ordered>(8, 64, AddOnePipe{}, MulPipe{});
auto p_copy = p;
auto p_moved = std::move(p_copy);

int out = p_moved.run(10);
```

## 8. 参考文件

- `core/pipe.hpp`
- `core/pipeline.hpp`
- `examples/pipeline_quickstart.cpp`

## 9. 阶段 B/C 新增能力（背压与可观测）

当前版本在 `NanoPipeLineOptions` 中新增了运行时治理配置：

- 共享池背压：
    - `shared_pool_queue_capacity`
    - `shared_pool_submit_policy`（`block` / `timeout` / `reject`）
    - `shared_pool_submit_timeout_ms`
- 专用池背压：
    - `dedicated_pool_queue_capacity`
    - `dedicated_pool_submit_policy`
    - `dedicated_pool_submit_timeout_ms`
- ordered 防护：
    - `ordered_waiters_limit`（防止 waiters 无上限增长）
    - `ordered_dispatch_budget`（分片调度预算，降低单次长占用）
- 运行时超时：
    - `run_timeout_ms`
- 事件回调：
    - `event_callback(const char* event_name, nanoai_u64 seq)`

可通过 `pipeline.stats()` 获取观测数据，包含：

- `run_submitted` / `run_completed`
- `run_rejected` / `run_timeout`
- `dispatch_rejected`
- `ordered_waiter_overflow`
- `in_flight_tasks`
- `next_sequence`

示例：

```cpp
NanoPipeLineOptions opts{};
opts.shared_pool_size = 8;
opts.global_task_quota = 128;
opts.shared_pool_queue_capacity = 512;
opts.shared_pool_submit_policy = ThreadPoolSubmitPolicy::timeout;
opts.shared_pool_submit_timeout_ms = 50;
opts.ordered_waiters_limit = 1024;
opts.run_timeout_ms = 2000;
opts.event_callback = [](const char* name, nanoai_u64 seq) {
        // 这里可接入日志、监控或 tracing 系统
};

auto p = make_pipeline<PipeForwardOrder::ordered>(opts, AddOnePipe{}, MulPipe{});
auto out = p.run(10);
auto s = p.stats();
```

## 10. 协作式取消与超时中断（阶段D）

### 10.1 协作式取消信号

- 所有 pipeline 支持传入 `NanoCancelToken`，用于外部取消任务。
- pipe 的 `on_run` 可选声明 `NanoCancelToken` 参数，框架会自动传递。
- 任务内部可定期检测 `cancel_token.is_cancelled()`，主动 return/throw。
- 协作式取消不会强制杀死线程，最终退出时机取决于任务何时检查取消信号。

### 10.2 用法示例

```cpp
#include "core/pipeline.hpp"

class CancellablePipe : public NanoAI_FLOW::NanoPipe<CancellablePipe>
{
public:
    int on_run(int x, const NanoCancelToken& cancel_token)
    {
        for (int i = 0; i < 100; ++i) {
            if (cancel_token.is_cancelled())
                throw std::runtime_error("cancelled");
            // ...执行部分工作...
        }
        return x + 1;
    }
};

NanoAI_FLOW::NanoPipeLineOptions opts;
CancellablePipe p;
auto pipeline = NanoAI_FLOW::make_pipeline<NanoAI_FLOW::PipeForwardOrder::ordered>(opts, p);
NanoCancelToken token;
// 另线程可调用 token.request_cancel();
try {
    pipeline.run_with_cancel(token, 42);
} catch (const std::exception& e) {
    // 捕获取消异常
}
```

### 10.3 超时中断

- pipeline 支持 run_timeout_ms，超时后自动抛出异常。
- 可结合 cancel_token 实现更细粒度的中断。

### 10.4 全局取消

- pipeline.shutdown() 或 request_cancel_all() 会全局发出取消信号。
- 所有在途任务收到信号后应协作退出；如任务不检查 token，则可能继续到自然完成。

### 10.5 注释规范

- 所有新增接口、参数、关键逻辑均严格遵循 comment_guidelines.md 注释要求。
