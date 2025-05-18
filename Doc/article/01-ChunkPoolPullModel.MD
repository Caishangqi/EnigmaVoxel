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

由于相邻的区块会有重复的面因此我们需要在一个区块加载完毕后通知其四周的区块进行重建，剔除不可见的区块相邻的顶点。同时当
玩家变更区块的`FBlock`后我们也需要重构区块的顶点，因此需要以下`Flags`去标记区块的“脏”状态。

- `bDirty`: 语义 = “本 Chunk 需要重建”，只能在
    - 邻块 `Loaded` 通知
    - 世界逻辑主动改动 block 时置为 `true`；在 排队那一刻 由 `FlushDirtyAndPending()` 清零
- `bQueuedForRebuild`：防抖。用 `exchange(true)` 保证同一 Chunk 在 Worker 跑完前不会重复入队
- `bNeedsNeighborNotify`：只在 完整生成 时才置 true，Mesh-Only 重建不再级联，消灭“无限传染”

都是 `std::atomic<bool>` ➜ 轻量；如果后面发现需要记录不同类型的脏（仅光照 / 仅 NavMesh …），可以把它们改成 `std::atomic<uint8>` 的 bitmask

## Cleaning up while queuing

- “排队时清脏” 而非 “Worker 清脏”：Worker 执行期间其他线程仍可能把 `bDirty` 拉高；
  只有 排队 那一刻把 flag 清零，才能保证 新的 脏不会被误抹掉

另一种做法是在 Worker 完成后，把脏位用 `CAS` 改成 “如果还是我当初那一轮的 tag 才清零”；但在 Unreal 的 GameThread / Worker 明确分帧的场景下没有必要

## GetBlock With Stage

`GetBlockAt…` 允许 `Ready` + `Loaded`，这样 `Ready` 时 Mesh 已生成，剔面算法可以安全查询体素；
提前允许可见性查询能消除“临界帧”黑缝

## Separate Management of Worker

线程池自己只管 Worker，队列由 World 掌控，这样方便做 “按需/带优先级” 调度，例如：

- 玩家正看着的区块 → 高优先级
- 远处预取区块 → 低优先级

若加入 IO 线程、压缩线程，可把队列再分层，`WorkerPool` 负责多级拉取