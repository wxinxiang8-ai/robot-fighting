# FreeRTOS 框架设计文档

> 轮式格斗机器人 - STM32F407VET6 主控

## 目录

- [一、整体架构概述](#一整体架构概述)
  - 1.1 三层架构模型
  - 1.2 核心设计原则
  - 1.3 时间预算分配
  - 1.4 FreeRTOS 配置要点
- [二、任务设计](#二任务设计)
  - 2.1 任务总览
  - 2.2 ControlTask（内环控制任务）
  - 2.3 DecisionTask（外环决策任务）
  - 2.4 DebugTask（调试监控任务）
  - 2.5 任务间通信与数据共享
  - 2.6 任务优先级与抢占关系
- [三、工程实现要点](#三工程实现要点)
  - 3.1 中断优先级配置
  - 3.2 portYIELD_FROM_ISR 立刻切换（待补充）
  - 3.3 ADC+DMA 数据一致性（待补充）
  - 3.4 2ms 内环禁止阻塞操作（待补充）

---

## 一、整体架构概述

### 1.1 三层架构模型

本系统采用**硬件中断层 → RTOS任务层 → 应用逻辑层**的三层架构，确保实时性与安全性的统一。

| 层级 | 职责 | 响应时间 | 典型组件 |
|------|------|----------|----------|
| **硬件中断层** | 时间基准、数据采集触发、紧急事件响应 | < 10μs | TIM6定时器中断、EXTI外部中断、DMA传输完成中断 |
| **RTOS任务层** | 业务逻辑调度、任务优先级管理、资源保护 | 100μs ~ 10ms | ControlTask、DecisionTask、DebugTask |
| **应用逻辑层** | 状态机、策略算法、上位机通信协议 | 10ms ~ 100ms | 登台状态机、漫游策略、串口命令解析 |

### 1.2 核心设计原则

#### 原则一：硬件定时器保证时间精度

- **TIM6** 作为控制内环的时间基准，固定 2ms 周期
- 中断服务程序（ISR）仅发送任务通知，不执行任何业务逻辑
- 控制任务通过 `ulTaskNotifyTake()` 阻塞等待，被唤醒后立即执行

#### 原则二：电机输出唯一控制者

- **只有 ControlTask 能直接操作电机 PWM**
- DecisionTask 仅产出"目标速度"，写入共享变量
- ControlTask 读取目标速度后，经过安全检查和 PID 计算，最终输出 PWM
- 彻底消除多任务竞态写入电机的风险

#### 原则三：边缘检测一票否决

- 边缘传感器状态在 ControlTask 内环中每 2ms 检查一次
- 一旦检测到边缘触发，**立即覆盖**外环下发的目标速度，强制执行后退/转向
- 安全逻辑与控制逻辑在同一任务中，无需跨任务通信，响应延迟最小

#### 原则四：ISR 最小化

- 所有中断服务程序遵循"只通知、不处理"原则
- 数据处理、逻辑判断全部放在任务中执行
- 避免在 ISR 中调用可能阻塞的 API

#### 原则五：数据快照 + 临界区保护

- 传感器数据采用快照机制，ControlTask 在每个周期开始时一次性读取所有传感器
- 共享变量访问使用 `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` 保护
- 关键数据结构附带时间戳，便于调试和数据融合

### 1.3 时间预算分配

系统总体时间预算基于 **2ms 控制周期**：

| 阶段 | 时间预算 | 说明 |
|------|----------|------|
| 传感器快照 | < 50μs | 读取边缘传感器、编码器、陀螺仪 |
| 边缘安全检查 | < 20μs | 判断是否触发边缘，决定是否覆盖目标 |
| PID 计算 | < 100μs | 双轮独立 PID 速度闭环 |
| PWM 输出 | < 10μs | 写入定时器比较寄存器 |
| **总计** | < 200μs | 占用率 < 10%，留足余量 |

### 1.4 FreeRTOS 配置要点

| 配置项 | 推荐值 | 说明 |
|--------|--------|------|
| `configTICK_RATE_HZ` | 1000 | 系统节拍 1kHz，时间精度 1ms |
| `configUSE_PREEMPTION` | 1 | 启用抢占式调度 |
| `configUSE_TIME_SLICING` | 0 | 关闭同优先级时间片轮转 |
| `configCHECK_FOR_STACK_OVERFLOW` | 2 | 启用栈溢出检测（方法2） |
| `configUSE_TASK_NOTIFICATIONS` | 1 | 启用任务通知（轻量级同步） |
| `configMINIMAL_STACK_SIZE` | 128 | 最小栈大小（单位：字） |

---

## 二、任务设计

### 2.1 任务总览

系统共设计 **3 个 RTOS 任务**，按优先级从高到低排列：

| 任务名称 | 优先级 | 触发方式 | 执行周期 | 栈大小 | 核心职责 |
|----------|--------|----------|----------|--------|----------|
| **ControlTask** | 3（最高） | TIM6中断通知 | 2ms | 512字 | 传感器快照、边缘安全检查、PID速度闭环、电机PWM输出 |
| **DecisionTask** | 2（中等） | vTaskDelayUntil | 20ms | 512字 | 状态机运行、策略决策、产出目标速度 |
| **DebugTask** | 1（最低） | vTaskDelayUntil | 100ms | 256字 | 串口调试输出、栈水位监控、性能统计 |

### 2.2 ControlTask（内环控制任务）

#### 2.2.1 任务定位

ControlTask 是系统的**实时控制核心**，负责所有与电机输出直接相关的操作。它是唯一能够写入电机 PWM 的任务，确保控制指令的一致性和安全性。

#### 2.2.2 触发机制

- **硬件定时器 TIM6** 每 2ms 产生一次中断
- 中断服务程序调用 `vTaskNotifyGiveFromISR()` 发送任务通知
- ControlTask 通过 `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` 阻塞等待
- 收到通知后立即唤醒，开始新一轮控制周期

#### 2.2.3 执行流程

ControlTask 每个周期按以下顺序执行：

1. **传感器快照**：一次性读取所有传感器数据（边缘传感器、编码器、陀螺仪），存入本地变量，避免多次读取导致数据不一致
2. **边缘安全检查**：判断是否有边缘传感器触发，若触发则进入"边缘规避模式"
3. **目标速度获取**：从共享变量中读取 DecisionTask 下发的目标速度（需临界区保护）
4. **安全覆盖逻辑**：若边缘触发，立即覆盖目标速度为"后退+转向"，忽略外环指令
5. **PID 计算**：根据目标速度和当前编码器反馈，计算左右轮的 PID 输出
6. **PWM 输出**：将 PID 输出值写入定时器比较寄存器，驱动电机

#### 2.2.4 数据输入

- **传感器数据**：边缘传感器状态（GPIO读取）、编码器计数值（定时器捕获）、陀螺仪角速度（SPI/I2C读取）
- **目标速度**：从共享变量 `g_TargetSpeed` 读取（DecisionTask 写入）

#### 2.2.5 数据输出

- **电机 PWM**：直接写入 TIM1/TIM8 的比较寄存器
- **实际速度**：将当前编码器计算出的实际速度写入共享变量 `g_ActualSpeed`（供 DecisionTask 参考）

#### 2.2.6 优先级说明

ControlTask 设置为**最高优先级（3）**，确保：
- 即使 DecisionTask 或 DebugTask 正在运行，TIM6 中断也能立即抢占
- 控制周期的时间精度得到保证
- 边缘检测的响应延迟最小化

---

### 2.3 DecisionTask（外环决策任务）

#### 2.3.1 任务定位

DecisionTask 是系统的**策略大脑**，负责根据当前状态和传感器信息，决定机器人应该执行什么动作，并产出目标速度指令。

#### 2.3.2 触发机制

- 使用 `vTaskDelayUntil()` 实现周期性执行，周期为 **20ms**
- 不依赖硬件中断，由 FreeRTOS 调度器在时间到达时唤醒

#### 2.3.3 执行流程

DecisionTask 每个周期按以下顺序执行：

1. **状态机更新**：根据当前状态（如"寻找台阶"、"登台中"、"台上漫游"、"攻击"）和传感器信息，决定下一个状态
2. **策略决策**：根据当前状态，调用对应的策略函数（如登台策略、漫游策略、攻击策略）
3. **目标速度计算**：策略函数产出目标线速度和角速度
4. **目标速度下发**：将目标速度写入共享变量 `g_TargetSpeed`（需临界区保护）

#### 2.3.4 数据输入

- **实际速度**：从共享变量 `g_ActualSpeed` 读取（ControlTask 写入）
- **传感器历史数据**：可选择性读取传感器快照（如需要做数据融合或滤波）
- **上位机指令**：从串口接收的命令（如切换模式、调整参数）

#### 2.3.5 数据输出

- **目标速度**：写入共享变量 `g_TargetSpeed`（ControlTask 读取）
- **当前状态**：写入共享变量 `g_CurrentState`（供 DebugTask 显示）

#### 2.3.6 优先级说明

DecisionTask 设置为**中等优先级（2）**，确保：
- 低于 ControlTask，不会干扰实时控制
- 高于 DebugTask，保证决策逻辑优先执行
- 20ms 周期足够完成状态机和策略计算，同时不会过度占用 CPU

#### 2.3.7 状态机设计要点

DecisionTask 的核心是状态机，典型状态包括：

- **IDLE（待机）**：等待启动信号
- **FIND_PLATFORM（寻找台阶）**：前进并检测台阶边缘
- **CLIMBING（登台中）**：执行登台动作序列
- **ON_PLATFORM（台上漫游）**：在台上随机移动，避免掉落
- **ATTACK（攻击）**：检测到对手后执行攻击策略
- **EMERGENCY（紧急停止）**：异常情况下的安全停止

状态转换由传感器事件、定时器、上位机指令等触发。

---

### 2.4 DebugTask（调试监控任务）

#### 2.4.1 任务定位

DebugTask 是系统的**调试助手**，负责输出调试信息、监控系统健康状态、响应上位机调试命令。

#### 2.4.2 触发机制

- 使用 `vTaskDelayUntil()` 实现周期性执行，周期为 **100ms**
- 周期较长，避免频繁串口输出影响实时性

#### 2.4.3 执行流程

DebugTask 每个周期按以下顺序执行：

1. **读取系统状态**：从共享变量读取当前状态、目标速度、实际速度、传感器状态等
2. **栈水位检测**：调用 `uxTaskGetStackHighWaterMark()` 检查各任务的栈使用情况
3. **性能统计**：记录 ControlTask 的执行时间、CPU 占用率等
4. **串口输出**：将调试信息格式化后通过 UART 发送到上位机
5. **命令解析**：检查串口接收缓冲区，解析上位机发来的调试命令（如调整 PID 参数、切换模式）

#### 2.4.4 数据输入

- **所有共享变量**：读取系统各模块的状态信息
- **串口接收缓冲区**：上位机发来的调试命令

#### 2.4.5 数据输出

- **串口调试信息**：通过 UART 发送到上位机
- **参数调整**：根据上位机命令，修改共享变量中的参数（如 PID 系数）

#### 2.4.6 优先级说明

DebugTask 设置为**最低优先级（1）**，确保：
- 不会干扰 ControlTask 和 DecisionTask 的实时性
- 只在 CPU 空闲时执行
- 串口输出的阻塞不会影响控制和决策

#### 2.4.7 调试信息格式

典型的调试输出格式（每 100ms 一行）：

```
[时间戳] 状态=ON_PLATFORM | 目标速度=(100,0) | 实际速度=(98,0) | 边缘=(0,0,0,0) | CPU=15%
```

便于上位机解析和绘制实时曲线。

---

### 2.5 任务间通信与数据共享

#### 2.5.1 共享变量设计

系统使用以下共享变量实现任务间通信：

| 变量名 | 类型 | 写入者 | 读取者 | 保护方式 |
|--------|------|--------|--------|----------|
| `g_TargetSpeed` | 速度结构体 | DecisionTask | ControlTask | 临界区 |
| `g_ActualSpeed` | 速度结构体 | ControlTask | DecisionTask | 临界区 |
| `g_CurrentState` | 枚举 | DecisionTask | DebugTask | 临界区 |
| `g_SensorSnapshot` | 传感器结构体 | ControlTask | DecisionTask | 临界区 |
| `g_PidParams` | PID参数结构体 | DebugTask | ControlTask | 临界区 |

#### 2.5.2 临界区保护原则

- 所有共享变量的读写必须在临界区内完成
- 使用 `taskENTER_CRITICAL()` 和 `taskEXIT_CRITICAL()` 成对调用
- 临界区内的代码尽可能短，避免长时间关闭中断
- 优先使用结构体整体拷贝，而非逐字段访问

#### 2.5.3 数据流向图

```
TIM6中断 ──通知──> ControlTask ──目标速度读取──> g_TargetSpeed <──目标速度写入── DecisionTask
                      │                                                      │
                      └──实际速度写入──> g_ActualSpeed ──实际速度读取────────┘
                      │
                      └──传感器快照写入──> g_SensorSnapshot ──传感器读取──> DecisionTask
                                                                              │
                                                                              └──状态写入──> g_CurrentState ──状态读取──> DebugTask
```

#### 2.5.4 避免死锁的设计

- 任务间不使用互斥量（Mutex），仅使用临界区
- 不存在"任务 A 等待任务 B，任务 B 等待任务 A"的循环依赖
- ControlTask 和 DecisionTask 之间通过共享变量单向传递数据，无阻塞等待

---

### 2.6 任务优先级与抢占关系

#### 2.6.1 抢占场景

1. **TIM6 中断抢占所有任务**：无论哪个任务正在运行，TIM6 中断都能立即响应
2. **ControlTask 抢占 DecisionTask**：若 DecisionTask 正在运行，ControlTask 被唤醒后立即抢占
3. **ControlTask 抢占 DebugTask**：若 DebugTask 正在运行，ControlTask 被唤醒后立即抢占
4. **DecisionTask 抢占 DebugTask**：若 DebugTask 正在运行，DecisionTask 被唤醒后立即抢占

#### 2.6.2 CPU 占用率估算

假设各任务的执行时间如下：

| 任务 | 执行时间 | 周期 | 占用率 |
|------|----------|------|--------|
| ControlTask | 200μs | 2ms | 10% |
| DecisionTask | 500μs | 20ms | 2.5% |
| DebugTask | 1ms | 100ms | 1% |
| **总计** | - | - | **13.5%** |

系统 CPU 占用率约 **13.5%**，留有充足余量应对突发情况和未来功能扩展。

---

## 三、工程实现要点

### 3.1 中断优先级配置

#### 3.1.1 FreeRTOS 中断优先级限制

在 STM32 + FreeRTOS 环境下，**并非所有中断都能调用 FreeRTOS API**。这是一个常见的工程陷阱，配置错误会导致系统崩溃或行为异常。

FreeRTOS 通过以下两个宏定义中断优先级范围：

| 配置项 | 典型值 | 说明 |
|--------|--------|------|
| `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` | 5 | 允许调用 `FromISR` API 的最高优先级（数值越小优先级越高） |
| `configLIBRARY_LOWEST_INTERRUPT_PRIORITY` | 15 | 最低中断优先级 |

**核心规则**：只有优先级数值 **≥ 5** 的中断才能调用 `vTaskNotifyGiveFromISR()`、`xQueueSendFromISR()` 等 FreeRTOS API。

#### 3.1.2 优先级配置错误的后果

| 错误配置 | 后果 |
|----------|------|
| TIM6 中断优先级设为 0-4 | 调用 `vTaskNotifyGiveFromISR()` 时系统崩溃或进入 HardFault |
| DMA 中断优先级设为 0-4 | 同上，数据传输完成通知失败 |
| 多个中断优先级相同 | 可能导致中断嵌套混乱，时序不可预测 |

#### 3.1.3 推荐的中断优先级配置

本系统中需要调用 FreeRTOS API 的中断及其推荐优先级：

| 中断源 | 优先级 | 是否调用 FromISR | 说明 |
|--------|--------|------------------|------|
| **TIM6**（控制内环） | 5 | 是 | 最高优先级，确保 2ms 控制周期精度 |
| **DMA**（ADC 传感器） | 6 | 是 | 传感器数据就绪通知 |
| **UART**（调试串口） | 7 | 是 | 接收中断，优先级最低 |
| **EXTI**（边缘传感器） | 5 | 可选 | 若需要立即通知任务则调用 FromISR |

#### 3.1.4 STM32CubeMX 配置方法

在 STM32CubeMX 的 NVIC 配置中：

1. 进入 **System Core → NVIC**
2. 找到对应的中断（如 TIM6_DAC_IRQn）
3. 将 **Preemption Priority** 设置为 5 或更大的数值
4. **Sub Priority** 可根据需要设置（通常为 0）

#### 3.1.5 代码中验证配置

可在系统初始化时添加断言，确保中断优先级配置正确：

```
若 TIM6 优先级 < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY，则触发断言失败
```

这样可以在开发阶段尽早发现配置错误，避免运行时崩溃。

---

### 3.2 portYIELD_FROM_ISR 立刻切换

#### 3.2.1 问题背景

当 TIM6 中断通过 `vTaskNotifyGiveFromISR()` 唤醒 ControlTask 后，**并不会立即切换到 ControlTask 执行**。默认情况下，FreeRTOS 会等到下一个系统节拍（Tick）才检查是否需要切换任务。

这会导致严重问题：
- 系统节拍周期为 1ms（configTICK_RATE_HZ = 1000）
- ControlTask 可能延迟 0~1ms 才被调度
- 2ms 的控制周期会产生 **最大 50% 的抖动**，严重影响控制精度

#### 3.2.2 解决方案

在中断服务程序末尾调用 `portYIELD_FROM_ISR()`，**强制触发一次任务调度**，确保高优先级任务立即执行。

#### 3.2.3 正确的 ISR 写法

TIM6 中断服务程序的标准写法：

```
void TIM6_DAC_IRQHandler(void) {
    // 1. 清除中断标志
    if (__HAL_TIM_GET_FLAG(&htim6, TIM_FLAG_UPDATE)) {
        __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);

        // 2. 定义切换标志变量
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // 3. 发送任务通知
        vTaskNotifyGiveFromISR(controlTaskHandle, &xHigherPriorityTaskWoken);

        // 4. 关键：根据标志决定是否立即切换
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
```

#### 3.2.4 工作原理

| 步骤 | 说明 |
|------|------|
| `xHigherPriorityTaskWoken = pdFALSE` | 初始化为"不需要切换" |
| `vTaskNotifyGiveFromISR(..., &xHigherPriorityTaskWoken)` | 如果被唤醒的任务优先级更高，FreeRTOS 会将此变量设为 pdTRUE |
| `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)` | 若变量为 pdTRUE，立即触发 PendSV 中断进行任务切换 |

#### 3.2.5 不加 portYIELD_FROM_ISR 的后果

| 场景 | 后果 |
|------|------|
| ControlTask 优先级最高 | 延迟 0~1ms 才执行，控制周期抖动严重 |
| 正在执行低优先级任务 | 低优先级任务继续执行直到下一个 Tick，高优先级任务被"饿死" |
| 边缘检测响应 | 边缘检测延迟增加，可能导致机器人掉台 |

#### 3.2.6 时序对比

**不加 portYIELD_FROM_ISR**：
```
TIM6中断 ──────► 返回低优先级任务 ──────► Tick中断 ──────► ControlTask
         │                              │
         └──────── 0~1ms 延迟 ──────────┘
```

**加了 portYIELD_FROM_ISR**：
```
TIM6中断 ──────► portYIELD ──────► ControlTask（立即执行）
         │                  │
         └──── < 10μs ──────┘
```

#### 3.2.7 其他需要 portYIELD_FROM_ISR 的场景

不仅是 TIM6，所有需要"立即唤醒任务"的中断都应该使用此模式：

| 中断 | 是否需要 portYIELD_FROM_ISR | 说明 |
|------|----------------------------|------|
| TIM6（控制内环） | ✅ 必须 | 2ms 周期精度要求高 |
| DMA 完成中断 | ✅ 推荐 | 传感器数据就绪后尽快处理 |
| UART 接收中断 | ⚠️ 可选 | 调试数据实时性要求不高 |
| EXTI 边缘中断 | ✅ 推荐 | 若用于紧急事件通知 |

---

### 3.3 ADC+DMA 数据一致性

#### 3.3.1 问题背景

ControlTask 的"传感器快照"需要在每个 2ms 周期开始时读取所有传感器数据。但如果 ADC+DMA 正在进行数据转换和传输，直接读取可能会得到**半更新的数据**——部分是上一帧，部分是当前帧。

这种数据不一致会导致：
- 边缘检测误判（四个灰度传感器数据不同步）
- PID 计算错误（编码器和目标速度时间戳不匹配）
- 系统行为不可预测

#### 3.3.2 问题场景示例

假设有 4 路 ADC 采集灰度传感器，DMA 正在传输：

```
DMA缓冲区: [LF] [RF] [LB] [RB]
              ↑
            DMA正在写入RF

此时 ControlTask 读取：
  LF = 新数据（本帧）
  RF = 旧数据（上一帧，DMA还没写完）  ← 数据不一致！
  LB = 旧数据（上一帧）
  RB = 旧数据（上一帧）
```

#### 3.3.3 解决方案概述

推荐采用 **TIM 触发 ADC + DMA 双缓冲** 方案：

| 方案 | 原理 | 优点 | 缺点 |
|------|------|------|------|
| **TIM 触发 ADC** | 定时器硬件触发 ADC 转换，与控制周期同步 | 采样时刻精确可控 | 需要配置定时器触发源 |
| **DMA 双缓冲** | 两个缓冲区交替使用，DMA 写一个，CPU 读另一个 | 彻底避免读写冲突 | 占用双倍 RAM |
| **DMA 完成中断** | 等 DMA 传输完成后再读取 | 简单可靠 | 需要同步机制 |

#### 3.3.4 方案一：TIM 触发 ADC（推荐）

让 TIM6 同时触发 ADC 转换和唤醒 ControlTask，确保数据采样与控制周期**硬件级同步**。

**工作流程**：
```
TIM6 溢出（2ms）
    │
    ├──► 触发 ADC 开始转换
    │
    └──► 发送任务通知唤醒 ControlTask
              │
              └──► ControlTask 等待 ADC 转换完成（约 10-50μs）
                        │
                        └──► 读取 DMA 缓冲区（数据已完整）
```

**STM32CubeMX 配置要点**：
1. ADC 设置：External Trigger = Timer 6 Trigger Out Event
2. ADC 设置：DMA Continuous Requests = Enable
3. TIM6 设置：Trigger Event Selection = Update Event

#### 3.3.5 方案二：DMA 双缓冲

使用两个缓冲区交替工作，彻底消除读写冲突。

**双缓冲机制**：
```
┌─────────────────────────────────────────────────────┐
│                    时间轴                            │
├─────────┬─────────┬─────────┬─────────┬─────────────┤
│  周期1  │  周期2  │  周期3  │  周期4  │    ...      │
├─────────┼─────────┼─────────┼─────────┼─────────────┤
│ DMA→A   │ DMA→B   │ DMA→A   │ DMA→B   │    ...      │
│ CPU←B   │ CPU←A   │ CPU←B   │ CPU←A   │    ...      │
└─────────┴─────────┴─────────┴─────────┴─────────────┘
        ↑ 永远不冲突：DMA 写 A 时，CPU 读 B
```

**缓冲区定义**：
```
#define ADC_CHANNEL_COUNT  6

// 双缓冲区
uint16_t g_adc_buffer[2][ADC_CHANNEL_COUNT];

// 当前可读的缓冲区索引（0 或 1）
volatile uint8_t g_adc_read_index = 0;
```

**DMA 完成中断中切换缓冲区**：
```
void DMA2_Stream0_IRQHandler(void) {
    if (DMA完成标志) {
        // 切换缓冲区索引
        g_adc_read_index = 1 - g_adc_read_index;

        // 通知 ControlTask 数据就绪
        vTaskNotifyGiveFromISR(...);
    }
}
```

#### 3.3.6 方案三：DMA 完成中断同步

最简单的方案：ControlTask 等待 DMA 完成中断后再读取数据。

**实现方式**：
```
// ControlTask 中
void ControlTask(void *pvParameters) {
    while (1) {
        // 等待 TIM6 通知
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // 等待 DMA 完成（使用信号量或任务通知）
        xSemaphoreTake(g_adc_complete_sem, pdMS_TO_TICKS(1));

        // 此时 DMA 已完成，数据一致
        Sensor_Snapshot();

        // ... 后续处理
    }
}
```

#### 3.3.7 数据快照的正确实现

无论采用哪种方案，ControlTask 中的数据快照都应该**一次性复制**所有数据到本地变量：

```
// 传感器快照结构体
typedef struct {
    uint16_t gray[4];       // 灰度传感器
    uint16_t ir_dist[2];    // 红外测距
    int32_t  encoder[2];    // 编码器
    uint32_t timestamp;     // 时间戳
} SensorSnapshot_t;

// 快照函数
void Sensor_Snapshot(SensorSnapshot_t *snapshot) {
    // 进入临界区，确保原子操作
    taskENTER_CRITICAL();

    // 一次性复制所有数据
    snapshot->gray[0] = g_adc_buffer[g_adc_read_index][0];
    snapshot->gray[1] = g_adc_buffer[g_adc_read_index][1];
    snapshot->gray[2] = g_adc_buffer[g_adc_read_index][2];
    snapshot->gray[3] = g_adc_buffer[g_adc_read_index][3];
    snapshot->ir_dist[0] = g_adc_buffer[g_adc_read_index][4];
    snapshot->ir_dist[1] = g_adc_buffer[g_adc_read_index][5];
    snapshot->encoder[0] = TIM2->CNT;
    snapshot->encoder[1] = TIM3->CNT;
    snapshot->timestamp = xTaskGetTickCount();

    taskEXIT_CRITICAL();
}
```

#### 3.3.8 推荐配置总结

| 项目 | 推荐配置 |
|------|----------|
| ADC 触发源 | TIM6 Update Event |
| DMA 模式 | Circular（循环模式） |
| 缓冲方案 | 双缓冲（内存充足时）或 DMA 完成同步 |
| 数据读取 | 临界区内一次性复制到本地快照 |
| 时间戳 | 每次快照附带 `xTaskGetTickCount()` |

---

### 3.4 2ms 内环禁止阻塞操作

#### 3.4.1 核心原则

ControlTask 的 2ms 控制周期内，**禁止任何可能阻塞或耗时不确定的操作**。一旦内环执行时间超过 2ms，会导致控制周期丢失、电机输出抖动、边缘检测延迟。

**时间预算回顾**：
- 总周期：2000μs
- 允许执行时间：< 200μs（占用率 < 10%）
- 剩余时间：留给任务切换、中断响应等

#### 3.4.2 禁止放入 ControlTask 的操作

| 操作类型 | 典型耗时 | 问题 | 替代方案 |
|----------|----------|------|----------|
| **I2C 读取**（陀螺仪） | 100-500μs | 阻塞等待 ACK，时间不确定 | DMA + 低优先级任务读取 |
| **SPI 读取**（传感器） | 50-200μs | 阻塞等待传输完成 | DMA + 缓冲区 |
| **串口发送**（调试） | 1-10ms | 阻塞等待发送完成 | DMA + 环形缓冲区 |
| **串口解析**（命令） | 不确定 | 依赖数据长度和复杂度 | DebugTask 中处理 |
| **长临界区** | 看情况 | 关中断时间过长 | 拆分为多次短临界区 |
| **浮点运算**（复杂） | 10-100μs | 无 FPU 时更慢 | 使用定点数或查表 |
| **动态内存分配** | 不确定 | malloc/free 时间不可预测 | 静态分配 |

#### 3.4.3 I2C/SPI 传感器的正确处理

**错误做法**（在 ControlTask 中阻塞读取）：
```
// ❌ 错误：阻塞等待 I2C 完成
void ControlTask(void) {
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ...);  // 阻塞 100-500μs
    // ... PID 计算
}
```

**正确做法**（DMA 后台读取 + 缓冲区）：
```
// ✅ 正确：后台 DMA 读取，ControlTask 只读缓冲区
// 低优先级任务中启动 DMA 读取
void SensorTask(void) {
    HAL_I2C_Mem_Read_DMA(&hi2c1, MPU6050_ADDR, ...);
}

// ControlTask 只读取已完成的缓冲区数据
void ControlTask(void) {
    int16_t gyro = g_gyro_buffer;  // 直接读取，< 1μs
    // ... PID 计算
}
```

#### 3.4.4 串口调试的正确处理

**错误做法**（在 ControlTask 中发送调试信息）：
```
// ❌ 错误：阻塞等待串口发送
void ControlTask(void) {
    printf("Speed: %d\n", speed);  // 阻塞 1-10ms
}
```

**正确做法**（DMA + 环形缓冲区 + DebugTask）：
```
// ✅ 正确：ControlTask 只写入缓冲区，DebugTask 负责发送
void ControlTask(void) {
    g_debug_data.speed = speed;  // 只更新数据，< 1μs
}

void DebugTask(void) {
    // 100ms 周期，使用 DMA 发送
    HAL_UART_Transmit_DMA(&huart1, buffer, len);
}
```

#### 3.4.5 临界区使用原则

| 原则 | 说明 |
|------|------|
| **尽可能短** | 单次临界区 < 10μs |
| **只保护必要数据** | 不要把计算逻辑放在临界区内 |
| **避免嵌套** | 不要在临界区内调用可能进入临界区的函数 |
| **优先用原子操作** | 单个 32 位变量可用原子读写代替临界区 |

**正确示例**：
```
// ✅ 临界区内只做数据拷贝
taskENTER_CRITICAL();
local_target = g_TargetSpeed;  // 拷贝，< 1μs
taskEXIT_CRITICAL();

// 计算放在临界区外
pid_output = PID_Calculate(local_target, actual);
```

#### 3.4.6 ControlTask 内允许的操作

| 操作 | 耗时 | 说明 |
|------|------|------|
| 读取 DMA 缓冲区 | < 5μs | 数据已就绪 |
| 读取编码器寄存器 | < 1μs | 直接读 TIMx->CNT |
| 读取 GPIO | < 1μs | 边缘传感器状态 |
| 整数 PID 计算 | < 50μs | 避免浮点除法 |
| 写 PWM 寄存器 | < 1μs | 直接写 TIMx->CCR |
| 短临界区 | < 10μs | 读写共享变量 |

#### 3.4.7 时间监控建议

在开发阶段，建议监控 ControlTask 的实际执行时间：

```
void ControlTask(void) {
    uint32_t start = DWT->CYCCNT;  // 使用 DWT 周期计数器

    // ... 控制逻辑 ...

    uint32_t elapsed = DWT->CYCCNT - start;
    uint32_t elapsed_us = elapsed / (SystemCoreClock / 1000000);

    if (elapsed_us > 200) {
        // 报警：执行时间超标
        g_control_overrun_count++;
    }
}
```

---

