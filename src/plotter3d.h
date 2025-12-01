#ifndef PLOTTER3D_H
#define PLOTTER3D_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include "meshdata.h" // 引用数据头文件
#include <QMatrix4x4> // <--- 必须加
#include <QVector3D>
#include <QPainter>


class Plotter3D : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
signals:
    // 信号：点被点击，线被点击
    void nodeClicked(int nodeId);
    void elementClicked(int elemId);

public:
    explicit Plotter3D(QWidget *parent = nullptr);

    // 传入数据指针
    void setMeshData(MeshData* data);
    void setHighlightIndices(const std::vector<int>& indices);
    void setHighlightElementIndices(const std::vector<int>& indices);
    void setShowFaceInfo(bool show);
protected:
    // --- OpenGL 核心三个函数 ---
    void initializeGL() override; // 初始化
    void resizeGL(int w, int h) override; // 窗口大小改变
    void paintGL() override;      // 每一帧绘制

    // --- 鼠标交互事件 ---
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;


private:
    void drawGrid();  // 辅助函数：画个地面对比
    void drawNodes(); // 核心函数：画点
    void drawLines(); // 画直线

    // 绘制面
    void drawFaces();
    // 绘制面信息
    bool m_showFaceInfo = false; // 开关变量，默认关闭
    void drawFaceInfo();


    // 生成弧线点
    std::vector<QVector3D> generateArcPoints(const Node& n1, const Node& n2, const Node& n3);
    void drawNodeIDs();

    MeshData* m_data = nullptr; // 数据源


    // 相机参数
    float m_xRot = 30.0f;    // X轴旋转角度
    float m_zRot = 0.0f;     // Z轴旋转角度（水平旋转）
    float m_zoom = -20.0f;   // 缩放距离（相机拉远拉近）
    QPoint m_lastPos;        // 鼠标最后位置

    float m_xPan = 0.0f;
    float m_yPan = 0.0f;

    QMatrix4x4 m_projection;
    QMatrix4x4 m_modelView;

    std::vector<int> m_highlightIDs;
    std::vector<int> m_highlightIndices;
    std::vector<int> m_highlightElementIndices; // 存高亮线的索引
    // 拾取函数
    int pickNode(const QPoint& mousePos);
    int pickLine(const QPoint& mousePos);
};

#endif // PLOTTER3D_H
