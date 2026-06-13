# Nexa — 高性能服务端 JS 运行时

开源项目（Apache 2.0）。Nexa = Nexus + Next。

## 定位

将 V8 嵌入 JVM 进程，Java/Kotlin 应用获得 Node.js 执行能力。
服务端优先，核心指标：RT < 10ms、内存可控、CPU 高效。

## 核心设计

- **Isolate 池** — 预热 V8 Isolate，请求即取即用
- **Snapshot 预热** — 预编译 bytecode + JIT 热点，冷启动 < 10ms
- **零拷贝** — DirectBuffer，不走 JSON 序列化
- **Shared RO Heap** — 所有 Isolate 共享 Node.js 标准库

## 技术栈

- C++ (V8 / libnode)
- JNI (Java/Kotlin) — 主战场
- CMake

## 架构文档

`/Users/sm3100/sunmi/ai-workspace/docs/designs/nexa/architecture.md`

## AI Workspace

- Profile: `nexa`
- 启动: `source /Users/sm3100/sunmi/ai-workspace/profiles/nexa.sh`
