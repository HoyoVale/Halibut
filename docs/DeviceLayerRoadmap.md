# HALIBUT Device 层最小设计草图

这份文档只回答一个问题：

`HALIBUTDevice` 在当前阶段应该负责什么，不应该负责什么，以及后续 `Buffer / Image` 应该怎样依赖它。

目标不是一步到位做成完整引擎，而是先把 GPU 入口层做稳。

## 1. 当前代码里，`HALIBUTDevice` 已经承担了什么

结合现有实现，`HALIBUTDevice` 目前已经承担了 4 类职责：

1. 物理设备发现和筛选
2. 逻辑设备与队列创建
3. 能力开关和扩展启用
4. 上传辅助设施

对应代码位置：

- 设备选择：`Halibut/src/Platform/Vulkan/HALIBUTDevice.cpp`
- 能力画像：`BuildCapabilityProfile()`
- 逻辑设备创建：`CreateLogicalDevice()`
- 即时提交和上传环：`CreateImmediateSubmitContext()`、`StageUploadData()`、`ImmediateSubmit()`

这个方向是对的。它说明 `HALIBUTDevice` 不是单纯的 `vk::Device` 包装，而是一个“GPU 入口对象”。

## 2. `HALIBUTDevice` 应该保留的职责

建议把 `HALIBUTDevice` 的稳定职责固定为下面 6 项。

### 2.1 设备选择

负责从候选 GPU 中选出一个可运行设备。

它需要做的事：

- 枚举 `PhysicalDevice`
- 检查图形队列和呈现队列
- 检查必要扩展
- 检查必要特性
- 保存最终选中的设备能力信息

这部分已经基本成型。

### 2.2 逻辑设备与队列

负责创建真正工作的 `LogicalDevice`，并拿到后续要使用的队列。

它需要暴露的信息通常包括：

- Graphics Queue
- Present Queue
- Queue Family Indices

后面 `Renderer`、`Swapchain`、上传通道、资源创建，都会依赖这些信息。

### 2.3 能力画像

建议把“设备支持什么”统一收敛到一个结构里，当前的 `HALIBUTDeviceCapabilityProfile` 就是这个方向。

这个结构建议继续保留，并逐步扩展：

- API 版本
- 扩展支持情况
- 动态渲染支持情况
- `samplerAnisotropy`
- 将来可以再加：
  - `descriptorIndexing`
  - `timelineSemaphore`
  - `bufferDeviceAddress`
  - `multiDrawIndirect`

注意：

不要把“创建资源时的策略判断”散落在 `Buffer.cpp`、`Image.cpp`、`Pipeline.cpp` 里。  
设备能力的判断尽量统一从 `HALIBUTDevice` 查。

### 2.4 内存查询

这是设备层最重要的基础服务之一。

它至少要稳定提供：

- `FindMemoryType(...)`
- `GetMemoryProperties()`

因为 `Buffer` 和 `Image` 的创建流程本质上都要经过：

1. 创建资源句柄
2. 查询 memory requirements
3. 找合适 memory type
4. 分配 memory
5. 绑定 memory

### 2.5 上传辅助

这是你当前实现里最值得保留的一部分。

目前你已经有：

- `BeginUploadBatch()`
- `EndUploadBatch()`
- `ImmediateSubmit(...)`
- `StageUploadData(...)`
- `CopyBufferImmediate(...)`
- `CopyBufferToImageImmediate(...)`

这套接口对后续 `Buffer / Image` 非常实用，因为它让资源类不需要自己管理一套上传命令池和 fence。

### 2.6 Pipeline Cache

这个职责继续放在 `Device` 里是合理的。

因为 pipeline cache 的生命周期通常跟逻辑设备强相关。

## 3. `HALIBUTDevice` 不应该负责的职责

为了后面抽成通用渲染层，下面这些不要继续往 `HALIBUTDevice` 里塞。

### 3.1 不负责 Swapchain 生命周期

`Device` 可以依赖 `Surface` 来选设备。  
但不应该负责：

- 创建 swapchain
- 重建 swapchain
- 管理 swapchain image views

这些属于呈现层。

### 3.2 不负责渲染流程

`BeginFrame()`、`BeginPass()`、`Present()` 这类事情属于 `Renderer` 或以后更高层的 `RenderContext`。

### 3.3 不负责资源对象的业务语义

例如：

- 顶点缓冲
- 索引缓冲
- 纹理
- 深度图
- 材质

这些不该放进 `Device`。  
`Device` 只提供创建它们所需的底层能力。

## 4. 面向后续开发，推荐保留的最小接口

当前阶段不建议一下子抽太大。  
只保留一套足够支撑 `Buffer / Image` 的最小接口即可。

```cpp
class HALIBUTDevice
{
public:
    // 设备与队列
    vk::raii::PhysicalDevice& GetPhysicalDevice();
    vk::raii::Device& GetDevice();
    vk::raii::Queue& GetGraphicsQueue();
    vk::raii::Queue& GetPresentQueue();
    QueueFamilyIndices& GetQueueFamilyIndices();

    // 能力与内存
    const HALIBUTDeviceCapabilityProfile& GetCapabilityProfile() const;
    const vk::PhysicalDeviceMemoryProperties& GetMemoryProperties() const;
    uint32_t FindMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties) const;

    // 上传辅助
    void BeginUploadBatch();
    void EndUploadBatch();
    void ImmediateSubmit(const std::function<void(vk::CommandBuffer)>& recordCommands);
    UploadBufferSlice StageUploadData(const void* sourceData, vk::DeviceSize size, vk::DeviceSize alignment = 16);
    void CopyBufferImmediate(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    void CopyBufferToImageImmediate(...);

    // 管线缓存
    vk::raii::PipelineCache& GetPipelineCache();
};
```

这套接口已经足够支撑下一阶段。

## 5. `Buffer` 应该怎样依赖 `Device`

后续 `HALIBUTBuffer` 的创建流程建议固定成下面这样。

### 5.1 创建流程

1. `HALIBUTBuffer` 接收 `HALIBUTBufferDesc`
2. 创建 `vk::Buffer`
3. 调 `getMemoryRequirements()`
4. 向 `HALIBUTDevice` 请求 `FindMemoryType(...)`
5. 分配 `vk::DeviceMemory`
6. `bindMemory`
7. 如果是 host visible，就允许 `Map / Write / Flush`

也就是说：

`Buffer` 自己负责“资源对象本体”，  
`Device` 负责“帮它找到能用的 GPU 环境”。

### 5.2 上传到 Device Local Buffer 的典型路径

如果以后你要做一个 GPU 本地的顶点缓冲，典型流程是：

1. 创建一个目标 buffer
   - `Usage = VertexBuffer | TransferDst`
   - `Memory = DeviceLocal`
2. 用 `device.StageUploadData()` 把 CPU 数据放进上传环
3. 用 `device.CopyBufferImmediate(...)` 或在一个上传批次里拷贝
4. 目标 buffer 留在 GPU 本地，供后续绘制使用

这里的关键理解是：

- 上传环是“施工临时仓库”
- 真正长期使用的顶点缓冲是另外一个 GPU buffer

## 6. `Image` 应该怎样依赖 `Device`

`HALIBUTImage` 的创建比 `Buffer` 多 3 个关键点：

1. 需要 `vk::ImageView`
2. 需要 layout transition
3. 上传纹理时通常要 `Buffer -> Image`

### 6.1 基础创建流程

1. 创建 `vk::Image`
2. 查 `memory requirements`
3. `device.FindMemoryType(...)`
4. 分配并绑定 memory
5. 创建 `vk::ImageView`

### 6.2 纹理上传流程

1. `device.StageUploadData()` 把像素数据写入上传环
2. 先把 image 从 `eUndefined` 转成 `eTransferDstOptimal`
3. `device.CopyBufferToImageImmediate(...)`
4. 再把 image 转成最终布局
   - 例如 `eShaderReadOnlyOptimal`

这里你现在还缺一块稳定能力：

- 通用的 image layout transition helper

这块可以先放在 `HALIBUTImage` 内部实现，  
后面如果资源越来越多，再考虑抽成独立 helper 或 command context。

## 7. 推荐的最小类关系

你当前阶段推荐维持下面这层关系：

```text
Instance
  -> Surface
  -> Device
      -> Buffer
      -> Image
      -> GraphicPipeline
  -> Swapchain
  -> Renderer
```

其中：

- `Surface` 主要服务于设备选择和 swapchain
- `Device` 主要服务于资源和命令提交辅助
- `Swapchain` 主要服务于呈现
- `Renderer` 主要服务于帧流程和 pass 流程

## 8. 你现在最该做的执行顺序

不要同时推进太多模块。  
推荐严格按下面顺序做。

### 第一步：收紧 `Device` 的定位

目标：

- 把 `HALIBUTDevice` 明确成 GPU 入口层
- 不继续往里面塞渲染流程逻辑

检查标准：

- 它不关心 `BeginFrame / EndFrame`
- 它不关心 swapchain recreation
- 它只提供设备、队列、能力、内存、上传辅助

### 第二步：完整实现 `HALIBUTBuffer`

目标：

- 完成创建、绑定、映射、写入、flush、invalidate

检查标准：

- 能创建 host visible buffer
- 能创建 device local buffer
- 能把 CPU 数据上传到 device local buffer

### 第三步：完整实现 `HALIBUTImage`

目标：

- 完成 image、memory、view、基础 layout transition、staging upload

检查标准：

- 能创建 2D 纹理
- 能上传一张图片
- 能在 shader 中采样

### 第四步：再回头收敛公共描述结构

例如：

- `BufferDesc`
- `ImageDesc`
- `SamplerDesc`
- `PipelineDesc`

这一步做完，你的 Vulkan backend 才会开始有“通用渲染 API”的味道。

## 9. 给新手的概念抓手

如果你做着做着又乱了，优先抓住这 4 个概念：

### 9.1 PhysicalDevice

它是“候选 GPU”。  
作用是查询支持什么，不负责真正创建资源。

### 9.2 LogicalDevice

它是“你和 GPU 的正式工作会话”。  
绝大多数资源、队列、同步对象都从它创建。

### 9.3 Queue Family

它是“GPU 的工种分组”。  
图形、呈现、传输，不一定在同一个 family。

### 9.4 Memory Type

它是“资源放在哪类内存里”。  
后面做 `Buffer / Image` 时，最容易出错也最值得花时间理解的就是它。

## 10. 一句话结论

你现在的正确路线不是继续往 `Renderer` 上堆功能，  
而是先把 `HALIBUTDevice` 固定成一个稳定的 GPU 入口对象，然后立刻用它完成 `HALIBUTBuffer` 和 `HALIBUTImage`。

这是最适合当前代码状态、也最适合新手逐步建立概念的一条路线。
