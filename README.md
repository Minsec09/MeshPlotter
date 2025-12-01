# MeshPlotter

项目中算法需要用一些点和边的数据来测试，为了快速获取测试数据，在gemini3pro的帮助下快速开发了这个绘图小工具，希望能帮到有该需求的人。

The algorithm in the project needs to be tested with some point and edge data. In order to quickly obtain test data, this small drawing tool was quickly developed with the help of Gemini3Pro. I hope it can help people who have this need.

这是一个用C++QT开发的绘图工具，支持三维空间打点、连线、画圆弧，以及渲染mesh。可用来快捷制作简单的点、边以及网格数据，支持简单的交互，支持导入导出到txt文档。

This is a drawing tool developed using C++ QT, supporting point plotting, line connection, arc drawing, and mesh rendering in 3D space. It can be used to quickly create simple point, edge, and mesh data, supports basic interaction, and allows importing and exporting to TXT documents.

# MeshPlotter / 三维网格编辑器



<img width="1604" height="1260" alt="image" src="https://github.com/user-attachments/assets/b3f9ea8d-ebb3-487e-9ff9-7e8e7af422e9" />



\*\*MeshPlotter\*\* is a tool application based on Qt 6 and OpenGL. It supports 3D node creation, wireframe connection, arc fitting, ray-picking interaction, and automatic mesh generation.



\*\*MeshPlotter\*\* 是一个基于 Qt 6 和 OpenGL 开发的小工具。它支持三维节点创建、线框连接、弧线拟合、射线拾取交互以及自动网格面生成。



---


## 📥 Download / 下载

You can download the ready-to-run package directly from the Releases page:
您可以直接从 Releases 页面下载可直接运行的压缩包：

👉 **[Download Latest Version / 下载最新版] (https://github.com/Minsec09/MeshPlotter/releases/tag/v1.0)**

---

## ✨ Features / 功能特性

*   **3D Interaction**: Rotate, Pan, Zoom, and Ray-Casting picking (Nodes & Edges).
    *   **三维交互**：支持旋转、平移、缩放以及光线投射拾取（点选和线选）。
*   **Geometry Editing**:
    *   Create Nodes (X, Y, Z).
    *   Connect Lines (Point-to-Point).
    *   **Arc Fitting**: Create arcs by selecting Start, Middle, and End points.
    *   **几何编辑**：创建节点、连接直线、**三点画弧**。
*   **Data Management**:
    *   Bi-directional sync between 3D View and Data Tables.
    *   ID Compacting (Automatic ID reordering after deletion).
    *   **数据管理**：3D视图与数据表格双向同步高亮，删除节点后自动重排 ID。
*   **Meshing**: Generate faces from closed loops using integrated geometric algorithms.
    *   **网格生成**：使用内置几何算法从闭合线框生成半透明网格面。
*   **IO**: Import/Export geometry data (.txt).
    *   **输入输出**：支持导入/导出几何数据文件。
