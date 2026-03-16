# 背景
Multi-Level Gearbox 是一个基于 NS-2 (Network Simulator 2) 的分层队列调度算法实现项目。该项目旨在通过模拟“齿轮箱”（Gearbox）的层级机制，对网络流量进行精细化调度，以在保证高吞吐量的同时提供低延迟和公平性。代码库包含了从 1 级到 5 级（GearboxOneLevel 到 GearboxFiveLevels）的不同实现，用于研究不同层级结构对网络性能的影响。

## 基础知识

### 0. 术语
- **Flow (流)**: 网络传输的基本逻辑单位，由 Flow ID (fid) 标识。每个流维护自己的状态（如权重、突发度）。
- **Level (层级)**: 调度器中的分层结构。Gearbox 包含多个 Level（如 1-5 层），层级越高通常优先级越高或处理不同特性的流量。
- **FIFO**: 每个 Level 内部包含多个 FIFO 队列，用于实际存储数据包。
- **Round (轮次)**: 调度器的时间单位或逻辑时钟，用于控制数据包的出队顺序。
- **Burstiness (突发度)**: 流的一种属性，用于决定新到达的数据包应该被插入到哪个 Level。

### 1. 基本架构
- **NS-2 扩展**: 本项目是 NS-2 的扩展模块，核心类继承自 NS-2 的 `Queue` 基类。
- **分层调度**:
    - 系统由多个 `Gearbox*Levels` 类组成（如 `GearboxTwoLevels`），每个类代表一种特定层级配置的调度器。
    - 内部通过 `std::vector<Level>` 维护多个层级。
    - 每个 `Level` 对象管理一组 `FIFO` 队列。
- **核心流程**:
    - **入队 (Enque)**: 数据包到达时，根据所属流的特性（如 `burstiness`）计算目标 Level 和 FIFO，并插入队列。
    - **出队 (Deque)**: 链路空闲时，调度器根据当前 Round 和层级优先级策略，选择一个 FIFO 进行出队。
    - **流管理**: 使用 `FlowMap` (`std::map<int, Flow*>`) 动态管理流状态，支持流的动态创建和查找。

### 2. 编译说明
- 项目依赖 NS-2 环境（通常为 ns-2.34）。
- **编译流程**:
    1.  **链接源码**: 将项目根目录下的 `.cc` 和 `.h` 文件（如 `gearbox-one-level.cc`, `level.cc`, `flow.cc`）软链接或复制到 NS-2 的 `queue` 目录下。
    2.  **修改构建配置**: 修改 NS-2 的 `Makefile` (在 `common/` 或根目录下)，将新的对象文件（如 `gearbox-one-level.o`）加入到编译列表中。
    3.  **执行编译**: 在 NS-2 根目录下运行 `make clean && make`。
- **运行仿真**:
    - 使用 `ns` 命令运行 TCL 脚本。入口脚本通常为 `tcp-HRCC.tcl`。
    - 示例命令: `ns tcp-HRCC.tcl <flow_num> <load> <topology> <algorithm>`

### 3. 代码模块说明
代码结构清晰，主要分为核心实现、公共组件和仿真脚本三部分。

- **核心调度模块** (根目录):
    - `gearbox-one-level.{cc,h}` ~ `gearbox-five-levels.{cc,h}`: Gearbox 调度算法的具体实现。不同文件对应不同的层级数量配置。这是重构后的新版代码（小写文件名）。
    - `original/`: 包含旧版本的实现（如 `Gearbox_one_level.cc`），保留用于参考或对比。建议优先关注根目录下的新版代码。
    - **变体实现**: 如 `Gearbox_pl_fid_flex_*level.cc`，针对特定拓扑（如 Parking Lot）或流 ID 策略优化的变体实现。

- **公共组件**:
    - `level.{cc,h}`: **Level 类**。封装了单层 Gearbox 的逻辑，管理内部的 FIFO 队列数组 (`std::vector<PacketQueue*>`) 和当前服务指针。
    - `flow.{cc,h}`: **Flow 类**。用于记录每个流的运行时信息，包括 `last_departure_round` (上一次出队轮次) 等，是调度决策的关键依据。

- **仿真脚本**:
    - `tcp-HRCC.tcl`: 主仿真脚本。配置网络拓扑、流量生成器、链路参数，并选择具体的 Gearbox 算法进行模拟。
    - `Topology-*.tcl`: 定义不同的网络拓扑结构（如 8 主机、16 主机）。
    - `common.tcl`: 通用的 TCL 辅助函数。

### 4. 线程模型
- **单线程模型**: NS-2 是基于离散事件驱动的单线程模拟器。
- **执行流**: 所有的包处理逻辑（`enque`, `deque`）、定时器回调均在同一个主线程中串行执行。
- **并发控制**: 由于是单线程环境，代码中不需要互斥锁（Mutex）或原子操作，数据结构（如 `std::deque`, `std::map`）的访问是线程安全的。

### 5. 产品形态
本项目主要以 **NS-2 仿真模块** 的形式存在，根据配置不同可表现为：
- **不同层级的调度器**: 通过 TCL 脚本中的 `bottleneckAlg` 参数，可以动态切换使用 1 级到 5 级的 Gearbox 算法（如 `GearboxOneLevel`, `GearboxTwoLevels` 等）。
- **仿真场景**: 支持多种网络拓扑（Dumbbell, Parking Lot 等，通过 Topology 文件定义）和流量模式（通过 `flow_benchmark` 目录下的脚本生成）。
