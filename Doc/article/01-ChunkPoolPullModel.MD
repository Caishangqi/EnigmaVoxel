# Chunk Pool Pull Model

## Chunk Stage

> Unloaded → Loading → Ready → Loaded → PendingUnload）

把 I/O/生成 → GameThread 拷贝 → 可以被查询/渲染 这三步拆开：

- `Loading`：异步线程独占，避免和渲染线程抢锁
- `Ready`：生成完但还没拷进 Actor，可随时被 GameThread 拾取
- `Loaded`：拷贝完成，Mesh 已经对渲染可见
- `PendingUnload`：引用计数归 0 后延迟删除，防止玩家快速来回导致反复加载

如果将来引入磁盘缓存或网格压缩，可以把 `Loading` 再细分出 “等待磁盘 I/O” 与 “CPU 生成” 两级

## Dirty / Rebuild / NeighborNotify

- `bDirty`: 语义 = “本 Chunk 需要重建”，只能在
    - 邻块 `Loaded` 通知
    - 世界逻辑主动改动 block 时置为 true；在 排队那一刻 由 FlushDirtyAndPending() 清零
- `bQueuedForRebuild`：防抖。用 exchange(true) 保证同一 Chunk 在 Worker 跑完前不会重复入队
- `bNeedsNeighborNotify`：只在 完整生成 时才置 true，Mesh-Only 重建不再级联，消灭“无限传染”

都是 `std::atomic<bool>` ➜ 轻量；如果后面发现需要记录不同类型的脏（仅光照 / 仅 NavMesh …），可以把它们改成 `std::atomic<uint8>` 的 bitmask