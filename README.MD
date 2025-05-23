﻿# Project: Enigma Voxel

![project-title-bg](https://github.com/user-attachments/assets/b4fe8d0d-e874-4977-aa1a-24aa8265515f)

<p align="center">
<img src = "https://i.imgur.com/EF6t6WA.png" alt="">
</p>

<h4 align="center">A Topdown voxel RPG game that focus on chunk system and world generation</h4>
<p align="center">
<a href="https://www.codefactor.io/repository/github/caishangqi/EnigmaVoxel"><img src="https://www.codefactor.io/repository/github/caishangqi/EnigmaVoxel/badge" alt="CodeFactor" /></a>
<img alt="Lines of code" src="https://img.shields.io/tokei/lines/github/Caishangqi/EnigmaVoxel">
<img alt="Lines of code" src="https://img.shields.io/badge/Unreal Engine-5.5-green">
<img alt="GitHub branch checks state" src="https://img.shields.io/github/checks-status/Caishangqi/EnigmaVoxel/master?label=build">
<img alt="GitHub code size in bytes" src="https://img.shields.io/github/languages/code-size/Caishangqi/EnigmaVoxel">
</p>

## 概述 Overview

EnigmaVoxel is a voxel-based terrain system for Unreal Engine 5.5, featuring dynamic chunk loading, hidden vertex/triangle culling, C++ core components, blueprint-derived block types, and Perlin
noise-based world generation.

<!-- TOC -->
* [Project: Enigma Voxel](#project-enigma-voxel)
  * [概述 Overview](#概述-overview)
  * [特性 Feature](#特性-feature)
  * [待优化 To be optimized](#待优化-to-be-optimized)
  * [里程碑 Milestone](#里程碑-milestone)
    * [Prototype: 基于范围的平面Chunk生成](#prototype-基于范围的平面chunk生成)
    * [Prototype: 多线程生成Chunk生成实现](#prototype-多线程生成chunk生成实现)
    * [Prototype: 区块加载后剔除相邻面](#prototype-区块加载后剔除相邻面)
  * [兴趣点 Point Of Interest](#兴趣点-point-of-interest)
  * [特别说明](#特别说明)
<!-- TOC -->

## 特性 Feature

> *斜体* 内容代表计划内容，非斜体代表已经完善的内容

- Block 注册系统和 Block 定义
- 基于蓝图的 Dynamic Mesh Actor 实现方块的添加,指定面剔除和添加材质槽
- 世界子系统 `UEnigmaWorldSubsystem`,区块 `UChunk`，和游戏世界 `UEnigmaWorld` 结构设计，包括其函数和生命周期管理
- 基于玩家当前位置动态加载 / 卸载 区块 (主线程)
- 抽象Chunk数据至`FChunkData`，包含所需要构建的`FDynamicMesh`和方块信息，这些内容可以被线程池抽离计算并在最后提交给主线程生成 `AChunk` Actor
- 方块局部坐标(区块) 和 世界坐标(世界) 直接的转换,方块的`Get()`函数

- *区块系统所需要的函数比如说 /setblock `<block>` `<xyz>`*
- *完整的区块系统, 包括区块数据结构, 破坏放置方块时更新区块顶点*
- *完整的破坏和放置机制, 基于事件去更新 Chunk 的所有顶点 (已不采用事件通知去更新区块加载完毕的Mesh重建)*
- *区块系统序列化, 创建可序列化的数据结构, 可在蓝图中编辑器中更新区块所包含的方块并且实时更新到视口中*
- *额外方块模型支持, 例如半砖, 竖砖块*
- _支持柏林噪声世界生成, 区块加载卸载_
  ...

## 待优化 To be optimized

- **(已完成)** *使用Unreal Engine的**多线程**管理去多线程计算区块数据逻辑，以及渲染的顶点(struct 数据结构)，虽然我们无法在生成和渲染时使用多线程 (UE 强制Spawn Actor和渲染使用主线程)*
- **(90%)** *实现**Chunk Pool**,当玩家离开Chunk视野范围时不直接进行区块卸载操作,先让Chunk `UnTick()` 并且不可见，随后在指定时间后真正地`Unload()`他们。*

## 里程碑 Milestone

不同阶段对应着我对项目的预期，同时不同阶段也会解决具体的问题，这些问题有着相同的技术点。一旦我突破他们，在下个原型 / 阶段 你将不会看到我对他们的描述。

### Prototype: 基于范围的平面Chunk生成

当前阶段拥有完整的方块注册系统，虽然还不支持方块UV和材质的渲染，但是已经支持方块的顶点生成，单个区块的重叠面优化。 同时世界子系统 `UEnigmaWorldSubsystem` 管理多个 `UEnigmaWorld` - 作为代理世界类代理原生
`UWorld`，保存当前世界加载的区块 / 在线的玩家 在每 0.05 秒一次检测中动态地基于 3x3 范围在玩家周围生成区块并且卸载区块。

<p align="center">
<img src = "https://github.com/Caishangqi/EnigmaVoxel/blob/5fafe235e8a87f850b4bd8df092dbd2a14f384a0/Doc/res/Milestone_Chunk_Flat_Generation.gif" alt="Milestone Chunk flat">
</p>

> 然而，该原型需要优化的地方很多，需要使用多线程技术去生成区块数据以及渲染顶点。同时还需要规定特殊地游戏机制避免在玩家离开范围后立刻删除区块，取而代之将这些方块放到 **额外的Buffer区域** 或者 **标记为不活跃
** 这样当玩家在短时间内回到视野范围外的区块是不用再次进行磁盘读取和 Actor 生成步骤。

### Prototype: 多线程生成Chunk生成实现

在这个里程碑，`AChunk`所有的区块数据被抽离至`FChunkHolder`。该数据结构包含 `FDynamicMesh` 构建`AChunk`时所需要的顶点和面以及所有的方块信息。随后基于玩家视野范围计算需要添加的区块构建任务给`ChunkWorkerPool`通过 `EnqueueBuildTask()`

<p align="center">
<img src = "https://github.com/Caishangqi/EnigmaVoxel/blob/53ec02f39083d8b1740c5533eeeead407e938c91/Doc/res/Milestone_Chunk_16x8_Asyn.gif" alt="Milestone Async Chunk generation">
</p>

> 当前阶段任然要处理渲染细节上的问题例如，当区块生成的时候是否要生成相接区域方块的面，这样可以保证在视觉效果上美观。
> 当加载完毕时则需重新更新一次FDynamicMesh来剔除那些在区块连接处的重叠的面。(不过这是纯美观层面)

### Prototype: 区块加载后剔除相邻面

当前阶段实现了区块在加载完毕后通知相邻区块`NotifyNeighborsChunkLoaded()`重新更新自己的vertices的步骤`RebuildChunkMeshData()`。这里没有使用`AChunk`来监听加载事件是因为在
`UEnigmaWorld`中我们能知道当前所有区块加载的状态而`AChunk`的生命周期往往要比`UEnigmaWorld`慢。(因为它是Unreal Engine的Actor并非纯数据对象)

<p align="center">
<img src = "https://github.com/Caishangqi/EnigmaVoxel/blob/6c45ca2dd1a3ae9145c32cd462228f6d4c61e696/Doc/res/Milestone_Chunk_neibour_culling.gif" alt="Milestone Chunk Neighbour Culling">
</p>

> 当前阶段没有性能问题但是仍然需要时刻注意这类问题的隐患，例如，如果一个方块被玩家破坏了，那么它周围的方块的顶点也需要重新计算，这可能会导致性能问题。不过该优化在贴图实现里程碑之后。

## 兴趣点 Point Of Interest
兴趣点是里程碑 **MileStone** 实现完毕后的技术总结，通常意味着项目确定使用该方案并且抛弃或者存档原本的技术方案(或许会保留原本的branch)

- [**Chunk Pool Pull Model 线程池拉模型**](https://github.com/Caishangqi/EnigmaVoxel/blob/master/Doc/article/01-ChunkPoolPullModel.MD) (参考了Minecraft的 Queue Chunk Pool)

## 特别说明

这是一个实验为准的项目，请不要对他保有太多的期待，主要内容会依照我觉得重要的部分进行开发，您的需求可能会被延后，感谢谅解。

> This is an experimental project, please don't have too much expectation on it. The main content will be developed according to the parts I think are important. Your needs may be delayed. Thank you
> for your understanding.
