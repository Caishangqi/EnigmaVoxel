# Project: Enigma Voxel 

![project-title-bg](https://github.com/user-attachments/assets/b4fe8d0d-e874-4977-aa1a-24aa8265515f)

<p align="center">
<img src = "https://i.imgur.com/EF6t6WA.png" alt="">
</p>

<h4 align="center">基于 Unreal Engine 5 实现体素多模型Mesh合并算法</h4>
<p align="center">
<a href="https://www.codefactor.io/repository/github/caishangqi/CharmingJourneyCore"><img src="https://www.codefactor.io/repository/github/caishangqi/charming-realm-system/badge" alt="CodeFactor" /></a>
<img alt="Lines of code" src="https://img.shields.io/tokei/lines/github/Caishangqi/CharmingJourneyCore">
<img alt="Lines of code" src="https://img.shields.io/badge/NeoForge-1.21.4-green">
<img alt="GitHub branch checks state" src="https://img.shields.io/github/checks-status/Caishangqi/CharmingJourneyCore/master?label=build">
<img alt="GitHub code size in bytes" src="https://img.shields.io/github/languages/code-size/Caishangqi/CharmingJourneyCore">
</p>

## 概述

基于 Unreal Engine 5.5 的 `DynamicMesh` 实现的区块加载和隐藏顶点/三角形剔除，核心业务逻辑包括 `Chunk`, `Block` 由C++实现，不同的方块类型则使用蓝图基类 `Block_C` 继承

## 特性

> *斜体* 内容代表计划内容，非斜体代表已经完善的内容

- Block 注册系统和 Block 定义
- 基于蓝图的 Dynamic Mesh Actor 实现方块的添加,指定面剔除和添加材质槽



- *完整的区块系统, 包括区块数据结构, 破坏放置方块时更新区块顶点*
- *完整的破坏和放置机制, 基于事件去更新 Chunk 的所有顶点*
- *区块系统序列化, 创建可序列化的数据结构, 可在蓝图中编辑器中更新区块所包含的方块并且实时更新到视口中*
- *额外方块模型支持, 例如半砖, 竖砖块*
- _支持柏林噪声世界生成, 区块加载卸载_
  ...



## 特别说明

这是一个专门给服务器使用的模组，主要需求会依照服务器需求进行配置，您的需求可能会被延后，感谢谅解。

> This is a plugin specially used for the server. The main requirements will be configured according to the server
> requirements. Your requirements may be delayed. Thank you for your understanding.
