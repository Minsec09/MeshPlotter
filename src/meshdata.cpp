#include "meshdata.h"
#include "geometry_utils.h"
// #include <algorithm>
#include <QDebug>

MeshData::MeshData() {}

int MeshData::addNode(double x, double y, double z) {
    Node n;
    n.id = m_nextNodeId++; // ID 从 0 开始
    n.x = x; n.y = y; n.z = z;
    m_nodes.push_back(n);
    return n.id;
}
void MeshData::removeNodeAtIndex(int index)
{
    if (index < 0 || index >= m_nodes.size()) return;

    // 1. 获取即将被删除的点的 ID
    int deletedId = m_nodes[index].id;

    // 2. 删除该点
    m_nodes.erase(m_nodes.begin() + index);

    // 3. 【关键】重排序：遍历剩下的点，所有 ID > deletedId 的都减 1
    for (auto& node : m_nodes) {
        if (node.id > deletedId) {
            node.id--;
        }
    }

    // 4. 【关键】更新线：遍历所有线，更新它们引用的节点 ID
    // 因为所有大于 deletedId 的节点 ID 都变了，线里面存的 ID 也要跟着变
    for (auto& elem : m_elements) {
        if (elem.startNodeId > deletedId) {
            elem.startNodeId--;
        }
        if (elem.endNodeId > deletedId) {
            elem.endNodeId--;
        }
    }

    // 5. 更新 ID 计数器
    // 因为 ID 是连续的，所以下一个 ID 就是当前的 size
    m_nextNodeId = m_nodes.size();
}
void MeshData::removeElementsConnectedTo(int nodeId)
{
    // 从后往前遍历，找到连着这个点的线，就删掉
    // 注意：必须调用 removeElementAtIndex，因为它里面包含了 "ID--" 的重排逻辑
    for (int i = m_elements.size() - 1; i >= 0; --i) {
        const auto& elem = m_elements[i];
        if (elem.startNodeId == nodeId || elem.endNodeId == nodeId) {
            removeElementAtIndex(i);
        }
    }
}

int MeshData::addLine(int startNodeId, int endNodeId) {
    Element e;
    e.id = m_nextElementId++;
    e.type = TYPE_LINE;
    e.startNodeId = startNodeId;
    e.endNodeId = endNodeId;
    // 直线不需要 mid 坐标，设为0即可
    m_elements.push_back(e);
    return e.id;
}

void MeshData::removeElementAtIndex(int index)
{
    if (index < 0 || index >= m_elements.size()) return;

    int deletedId = m_elements[index].id;

    m_elements.erase(m_elements.begin() + index);

    // 重排序剩余的线
    for (auto& elem : m_elements) {
        if (elem.id > deletedId) {
            elem.id--;
        }
    }

    // 更新线 ID 计数器
    m_nextElementId = m_elements.size();
}


int MeshData::addArc(int startNodeId, int endNodeId, double midX, double midY, double midZ) {
    Element e;
    e.id = m_nextElementId++;
    e.type = TYPE_ARC;
    e.startNodeId = startNodeId;
    e.endNodeId = endNodeId;
    e.midX = midX;
    e.midY = midY;
    e.midZ = midZ;
    m_elements.push_back(e);
    return e.id;
}
void MeshData::generateFaces()
{
    // --- 1. 准备数据 (Data Adapting) ---

    // A. 转换点数据
    std::vector<std::array<double, 3>> input_points;
    input_points.reserve(m_nodes.size());

    for (const auto& node : m_nodes) {
        input_points.push_back({node.x, node.y, node.z});
    }

    // B. 转换边数据
    std::vector<std::array<int, 3>> input_edges;
    std::vector<std::array<double, 3>> input_edges_info;

    input_edges.reserve(m_elements.size());
    input_edges_info.reserve(m_elements.size());

    for (const auto& elem : m_elements) {
        // 构造 edges 参数: [start, end, isArc]
        int isArc = (elem.type == TYPE_ARC) ? 1 : 0;
        input_edges.push_back({elem.startNodeId, elem.endNodeId, isArc});

        // 构造 edges_info 参数: [midX, midY, midZ] (直线填0)
        if (isArc) {
            input_edges_info.push_back({elem.midX, elem.midY, elem.midZ});
        } else {
            input_edges_info.push_back({0.0, 0.0, 0.0});
        }
    }

    // --- 2. 调用第三方库 ---
    // 使用 try-catch 防止库内部崩溃导致软件闪退
    try {
        auto result = cgal_tools::reconstruct_meshes(input_points, input_edges, input_edges_info);

        // --- 3. 解析结果存回 MeshData ---
        m_faces.clear();

        const auto& result_indices = result.first;      // 面包含的点索引
        const auto& result_props = result.second;       // 面的属性

        for (size_t i = 0; i < result_indices.size(); ++i) {
            Face face;
            face.nodeIndices = result_indices[i]; // 拷贝 vector<int>

            // 拷贝属性
            if (i < result_props.size()) {
                face.area = result_props[i].area;
                face.centerX = result_props[i].center_x;
                face.centerY = result_props[i].center_y;
                face.centerZ = result_props[i].center_z;
            }

            m_faces.push_back(face);
        }

    } catch (const std::exception& e) {
        // 可以在这里打印日志
        qDebug() << "Error in mesh reconstruction:" << e.what();
    } catch (...) {
        qDebug() << "Unknown error in mesh reconstruction.";
    }
}

const std::vector<Node>& MeshData::getNodes() const {
    return m_nodes;
}

const std::vector<Element>& MeshData::getElements() const {
    return m_elements;
}

void MeshData::clearData(){
    // qDebug("clear data");
    m_nodes.clear();
    m_elements.clear();
    m_faces.clear();
    m_nextNodeId = 0;
    m_nextElementId = 0;
}
