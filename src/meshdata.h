#ifndef MESHDATA_H
#define MESHDATA_H

#include <vector>


enum ElementType {
    TYPE_LINE = 0,
    TYPE_ARC = 1
};

struct Node {
    int id;
    double x, y, z;
};

struct Face {
    std::vector<int> nodeIndices; // 面的组成点索引
    double area;
    double centerX, centerY, centerZ;
};

// 单元（线/弧）
struct Element {
    int id;
    ElementType type; // 0=直线, 1=弧线

    // 拓扑连接
    int startNodeId;
    int endNodeId;
    double midX = 0.0;
    double midY = 0.0;
    double midZ = 0.0;

    // double centerX, centerY, centerZ;
};

class MeshData
{
public:
    MeshData();

    // 1. 添加节点
    int addNode(double x, double y, double z);
    // 删除节点
    void removeNodeAtIndex(int index);
    void removeElementsConnectedTo(int nodeId);

    // 2. 添加直线
    int addLine(int startNodeId, int endNodeId);
    // 删除线
    void removeElementAtIndex(int index);

    // 3. 添加弧线 (需要第三个点)
    int addArc(int startNodeId, int endNodeId, double midX, double midY, double midZ);

    // 生成面
    void generateFaces();

    void clearData();

    // Getters
    const std::vector<Node>& getNodes() const;
    const std::vector<Element>& getElements() const; // 新增获取所有线
    const std::vector<Face>& getFaces() const { return m_faces; }

private:
    std::vector<Node> m_nodes;
    std::vector<Element> m_elements;
    std::vector<Face> m_faces; // 存储生成的面

    int m_nextNodeId = 0;    // 节点ID计数
    int m_nextElementId = 0; // 单元ID计数 (建议分开计数)
};

#endif // MESHDATA_H
