#include "plotter3d.h"
#include <GL/gl.h> // 引入基础GL头文件
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


Plotter3D::Plotter3D(QWidget *parent) : QOpenGLWidget(parent)
{
    // 设置焦点，为了接收键盘事件（如果有的话）
    setFocusPolicy(Qt::StrongFocus);
}

void Plotter3D::setMeshData(MeshData *data)
{
    m_data = data;
}

void Plotter3D::initializeGL()
{
    initializeOpenGLFunctions(); // 初始化GL函数
    glEnable(GL_DEPTH_TEST);     // 开启深度测试（近遮远）
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // 背景设为深灰色

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // 深灰背景
}

void Plotter3D::resizeGL(int w, int h)
{
    // 设置视口
    glViewport(0, 0, w, h);

    // 设置投影矩阵 (透视投影)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 透视参数：FOV 45度，宽高比，近裁剪面0.1，远裁剪面1000
    double aspectRatio = double(w) / double(h ? h : 1);
    double zNear = 0.1, zFar = 1000.0;

    // 手动创建一个简单的透视矩阵 (或者用 QMatrix4x4 处理也可以)
    // 这里用 gluPerspective 的替代写法
    double fH = tan(45.0 / 360.0 * 3.14159265358979323846) * zNear;
    double fW = fH * aspectRatio;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}
void Plotter3D::setShowFaceInfo(bool show)
{
    m_showFaceInfo = show;
    update(); // 【关键】状态改变后，立即触发重绘
}
void Plotter3D::paintGL()
{
    // 1. 清除屏幕和深度缓存
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 设置模型视图矩阵 (相机位置)
    // --- 设置投影 ---
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 重新计算投影参数 (需与 resizeGL 逻辑一致，或者直接保存 resizeGL 里的矩阵)
    double aspectRatio = double(width()) / double(height() ? height() : 1);
    // 这里我们用 QMatrix4x4 手动算一下存起来，方便后面用
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, aspectRatio, 0.1f, 1000.0f);
    glLoadMatrixf(m_projection.constData()); // 应用给 OpenGL

    // --- 设置模型视图 ---
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 1. 先应用平移 (Pan)
    // 注意：我们将平移加在这里
    // m_zoom 是 Z 轴的平移，m_xPan/m_yPan 是 XY 轴的平移
    glTranslatef(m_xPan, m_yPan, m_zoom);

    // 2. 再应用旋转 (Rotate)
    glRotatef(m_xRot, 1.0f, 0.0f, 0.0f);
    glRotatef(m_zRot, 0.0f, 0.0f, 1.0f);

    // 保存矩阵供拾取使用 (保持不变)
    glGetFloatv(GL_MODELVIEW_MATRIX, m_modelView.data());
    // 如果你用的是手动算的 m_modelView，记得把 translate 也加进去：
    // m_modelView.translate(m_xPan, m_yPan, m_zoom);


    // 3. 开始绘制
    drawGrid();  // 画网格背景
    drawFaces();
    drawLines();
    drawNodes(); // 画我们自己的数据点

    drawNodeIDs();
    if (m_showFaceInfo) {
        drawFaceInfo();
    }
}
int Plotter3D::pickNode(const QPoint& mousePos)
{
    if (!m_data) return -1;

    const auto& nodes = m_data->getNodes();
    int closestId = -1;
    double minDist = 20.0; // 捕捉半径（像素），比如 20px 以内都算点中

    // 获取视口大小
    QRect viewport(0, 0, width(), height());

    for (const auto& node : nodes) {
        QVector3D objPos(node.x, node.y, node.z);

        // 【核心】将 3D 坐标投影到屏幕 2D 坐标
        // map 方法会自动处理 Viewport 变换，注意 Y 轴方向可能需要反转
        QVector3D screenPos = objPos.project(m_modelView, m_projection, viewport);

        // OpenGL 的 Y 是从下往上，Qt 鼠标是从上往下，project 算出来的是 OpenGL 坐标
        // 但 QVector3D::project 在 Qt6 中通常已经处理了坐标系。
        // 我们计算平面距离：
        // 注意：mousePos.y() 需要视情况反转，或者直接算距离

        // 修正：Qt 的 project 算出来的 Y 是屏幕坐标 (0在上方)，通常可以直接用
        double dx = mousePos.x() - screenPos.x();
        double dy = mousePos.y() - screenPos.y(); // 这里的 screenPos.y 可能需要 height() - y

        // 实际上 Qt project 算出的 screenPos.y 原点在左下角 (OpenGL标准)
        // 而 mousePos 原点在左上角
        double fixedScreenY = height() - screenPos.y();

        double dist = std::sqrt(dx*dx + (mousePos.y() - fixedScreenY) * (mousePos.y() - fixedScreenY));

        if (dist < minDist) {
            minDist = dist;
            closestId = node.id;
        }
    }
    return closestId;
}
void Plotter3D::drawGrid()
{
    // 画一个简单的地面网格
    glColor3f(0.4f, 0.4f, 0.4f); // 灰色线
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; ++i) {
        // 平行于X轴的线
        glVertex3f(-10.0f, i * 1.0f, 0.0f);
        glVertex3f( 10.0f, i * 1.0f, 0.0f);
        // 平行于Y轴的线
        glVertex3f(i * 1.0f, -10.0f, 0.0f);
        glVertex3f(i * 1.0f,  10.0f, 0.0f);
    }
    glEnd();

    // 画坐标轴
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0); glVertex3f(0,0,0); glVertex3f(2,0,0); // X红
    glColor3f(0.0, 1.0, 0.0); glVertex3f(0,0,0); glVertex3f(0,2,0); // Y绿
    glColor3f(0.0, 0.0, 1.0); glVertex3f(0,0,0); glVertex3f(0,0,2); // Z蓝
    glEnd();
}

void Plotter3D::setHighlightIndices(const std::vector<int>& ids) // 参数改名 ids
{
    m_highlightIDs = ids; // 存下来
    update();
}

// 修改原有的 drawNodes 函数
void Plotter3D::drawNodes()
{
    if (!m_data) return;
    const auto& nodes = m_data->getNodes();

    glPointSize(8.0f);
    glBegin(GL_POINTS);

    for (const auto& node : nodes) {
        // 【核心修复】这里比对的是 node.id，而不是循环变量 i
        bool isSelected = std::find(m_highlightIDs.begin(), m_highlightIDs.end(), node.id)
                          != m_highlightIDs.end();

        if (isSelected) {
            glColor3f(0.0f, 1.0f, 0.0f); // 绿
        } else {
            glColor3f(1.0f, 1.0f, 0.0f); // 黄
        }
        glVertex3f(node.x, node.y, node.z);
    }
    glEnd();
}

std::vector<QVector3D> Plotter3D::generateArcPoints(const Node& n1, const Node& n2, const Node& n3)
{
    std::vector<QVector3D> points;
    QVector3D p1(n1.x, n1.y, n1.z);
    QVector3D p2(n2.x, n2.y, n2.z);
    QVector3D p3(n3.x, n3.y, n3.z);

    // 1. 共线检查
    QVector3D v1 = p2 - p1;
    QVector3D v2 = p3 - p1;
    if (QVector3D::crossProduct(v1, v2).length() < 0.001f) {
        points.push_back(p1); points.push_back(p2); points.push_back(p3);
        return points;
    }

    // 2. 计算圆心 (经典公式)
    QVector3D normal = QVector3D::crossProduct(v1, v2); // 法向量（未归一化）
    float nSq = normal.lengthSquared();
    QVector3D temp = v1.lengthSquared() * v2 - v2.lengthSquared() * v1;
    QVector3D centerOffset = QVector3D::crossProduct(temp, normal) / (2.0f * nSq);
    QVector3D center = p1 + centerOffset;
    float radius = centerOffset.length();

    // 3. 构建局部坐标系 (Basis)
    QVector3D X = (p1 - center).normalized();            // X轴指向起点
    QVector3D Z = normal.normalized();                   // Z轴是法线
    QVector3D Y = QVector3D::crossProduct(Z, X).normalized(); // Y轴垂直于X

    // 4. 计算角度 (在 X-Y 平面上)
    // 起点肯定是 0度，因为 X轴就是 center->p1
    double angStart = 0.0;

    // 计算中间点角度
    QVector3D vecP2 = (p2 - center).normalized();
    double angMid = std::atan2(QVector3D::dotProduct(vecP2, Y), QVector3D::dotProduct(vecP2, X));

    // 计算终点角度
    QVector3D vecP3 = (p3 - center).normalized();
    double angEnd = std::atan2(QVector3D::dotProduct(vecP3, Y), QVector3D::dotProduct(vecP3, X));

    // 5. 强制角度顺序： Start(0) < Mid < End
    // 这样保证我们画的弧线一定是从 1 经过 2 到 3
    if (angMid < 0) angMid += 2.0 * M_PI; // 确保 Mid 是正的
    if (angEnd < 0) angEnd += 2.0 * M_PI; // 确保 End 是正的

    // 如果 End 比 Mid 小，说明转过头了，要加一圈
    if (angEnd < angMid) angEnd += 2.0 * M_PI;

    // 6. 插值生成点
    int segments = 40;
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / segments;
        // 在 Start 和 End 之间插值
        float ang = angStart + t * (angEnd - angStart);

        // 转回 3D 坐标
        QVector3D pt = center + radius * (std::cos(ang) * X + std::sin(ang) * Y);
        points.push_back(pt);
    }

    return points;
}
// --- 交互逻辑 ---

void Plotter3D::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->position().toPoint();

    if (event->button() == Qt::LeftButton) {
        // 策略：优先选点，如果点没选中，再试着选线
        int pickedNodeId = pickNode(m_lastPos);

        if (pickedNodeId != -1) {
            emit nodeClicked(pickedNodeId);
        } else {
            // 如果没点中点，尝试点线
            int pickedElemId = pickLine(m_lastPos);
            if (pickedElemId != -1) {
                emit elementClicked(pickedElemId); // 发射新信号
            }
        }
    }
}

void Plotter3D::mouseMoveEvent(QMouseEvent *event)
{
    // 获取当前点
    QPoint currentPos = event->position().toPoint();
    int dx = currentPos.x() - m_lastPos.x();
    int dy = currentPos.y() - m_lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        // 左键：旋转 (保持不变)
        m_xRot += dy;
        m_zRot += dx;
    }
    else if (event->buttons() & Qt::RightButton) {
        float sensitivity = 0.05f;
        m_xPan += dx * sensitivity;
        m_yPan -= dy * sensitivity;
    }

    m_lastPos = currentPos;
    update();
}

void Plotter3D::wheelEvent(QWheelEvent *event)
{
    // 滚轮：缩放
    float delta = event->angleDelta().y() / 120.0f;
    m_zoom += delta;
    update();
}
void Plotter3D::setHighlightElementIndices(const std::vector<int>& indices)
{
    m_highlightElementIndices = indices;
    update();
}

// --- 2. 修改 drawLines 支持高亮 ---
void Plotter3D::drawLines()
{
    if (!m_data) return;
    const auto& nodes = m_data->getNodes();
    const auto& elements = m_data->getElements();

    // 查找节点的 Lambda
    auto findNode = [&](int id) -> const Node* {
        for (const auto& n : nodes) if (n.id == id) return &n;
        return nullptr;
    };

    // 遍历所有单元
    for (size_t i = 0; i < elements.size(); ++i) {
        const auto& elem = elements[i];

        const Node* n1 = findNode(elem.startNodeId);
        const Node* n2 = findNode(elem.endNodeId);
        if (!n1 || !n2) continue;

        // --- 1. 先决定样式 (高亮/颜色) ---
        // 检查索引 i 是否在高亮列表中
        bool isSelected = std::find(m_highlightElementIndices.begin(), m_highlightElementIndices.end(), static_cast<int>(i))
                          != m_highlightElementIndices.end();

        if (isSelected) {
            glLineWidth(4.0f);           // 选中：粗线
            glColor3f(0.0f, 1.0f, 0.0f); // 选中：纯绿色
        } else {
            glLineWidth(2.0f);           // 普通：细线
            glColor3f(0.0f, 1.0f, 1.0f); // 普通：青色 (Cyan)
        }

        // --- 2. 再决定画什么形状 ---
        if (elem.type == TYPE_LINE) {
            // 画直线
            glBegin(GL_LINES);
            glVertex3f(n1->x, n1->y, n1->z);
            glVertex3f(n2->x, n2->y, n2->z);
            glEnd();
        }
        else if (elem.type == TYPE_ARC) {
            // 画弧线
            Node nMid;
            nMid.x = elem.midX;
            nMid.y = elem.midY;
            nMid.z = elem.midZ;

            std::vector<QVector3D> arcPts = generateArcPoints(*n1, nMid, *n2);

            // 注意：弧线是由多段小直线组成的，所以用 LINE_STRIP
            glBegin(GL_LINE_STRIP);
            for (const auto& pt : arcPts) {
                glVertex3f(pt.x(), pt.y(), pt.z());
            }
            glEnd();
        }
    }
}

// --- 3. 拾取线的核心算法 ---
int Plotter3D::pickLine(const QPoint& mousePos)
{
    if (!m_data) return -1;
    const auto& nodes = m_data->getNodes();
    const auto& elements = m_data->getElements();

    int closestId = -1;
    double minDist = 10.0; // 容差像素
    QRect viewport(0, 0, width(), height());

    auto findNode = [&](int id) -> const Node* {
        for (const auto& n : nodes) if (n.id == id) return &n;
        return nullptr;
    };

    for (const auto& elem : elements) {
        const Node* n1 = findNode(elem.startNodeId);
        const Node* n2 = findNode(elem.endNodeId);
        if (!n1 || !n2) continue;

        // 生成这一段的所有点 (如果是直线就是2个点，弧线就是40个点)
        std::vector<QVector3D> checkPoints;

        if (elem.type == TYPE_LINE) {
            checkPoints.push_back(QVector3D(n1->x, n1->y, n1->z));
            checkPoints.push_back(QVector3D(n2->x, n2->y, n2->z));
        }
        else if (elem.type == TYPE_ARC) {
            Node nMid; nMid.x = elem.midX; nMid.y = elem.midY; nMid.z = elem.midZ;
            checkPoints = generateArcPoints(*n1, nMid, *n2);
        }

        // 遍历所有小线段，计算距离
        for (size_t k = 0; k < checkPoints.size() - 1; ++k) {
            QVector3D p1_3d = checkPoints[k];
            QVector3D p2_3d = checkPoints[k+1];

            QVector3D s1 = p1_3d.project(m_modelView, m_projection, viewport);
            QVector3D s2 = p2_3d.project(m_modelView, m_projection, viewport);

            QPointF p1(s1.x(), height() - s1.y());
            QPointF p2(s2.x(), height() - s2.y());
            QPointF mouse(mousePos);

            double l2 = (p1.x()-p2.x())*(p1.x()-p2.x()) + (p1.y()-p2.y())*(p1.y()-p2.y());
            if (l2 == 0) continue;

            double t = ((mouse.x() - p1.x()) * (p2.x() - p1.x()) + (mouse.y() - p1.y()) * (p2.y() - p1.y())) / l2;
            t = std::max(0.0, std::min(1.0, t));

            QPointF projection = p1 + t * (p2 - p1);
            double dist = std::hypot(mouse.x() - projection.x(), mouse.y() - projection.y());

            if (dist < minDist) {
                minDist = dist;
                closestId = elem.id;
            }
        }
    }
    return closestId;
}

void Plotter3D::drawNodeIDs()
{
    if (!m_data) return;

    QPainter painter(this);
    painter.setPen(Qt::white); // 设置文字颜色
    painter.setFont(QFont("Arial", 10)); // 设置字体

    const auto& nodes = m_data->getNodes();
    QRect viewport(0, 0, width(), height());

    for (const auto& node : nodes) {
        //将 3D 坐标转换为屏幕 2D 坐标
        QVector3D objPos(node.x, node.y, node.z);

        // 使用在这个函数之前 paintGL 里计算好的矩阵
        // 注意：确保 m_modelView 和 m_projection 是最新的
        QVector3D screenPos = objPos.project(m_modelView, m_projection, viewport);

        // 检查点是否在相机后面
        if (screenPos.z() > 1.0f) continue;

        // 坐标修正
        double x = screenPos.x();
        double y = height() - screenPos.y();

        // 绘制文字
        painter.drawText(QPointF(x + 5, y - 5), QString::number(node.id));
    }

}

void Plotter3D::drawFaces()
{
    if (!m_data) return;
    const auto& nodes = m_data->getNodes();

    const auto& faces = m_data->getFaces();
    auto findNode = [&](int id) -> const Node* {
        for (const auto& n : nodes) if (n.id == id) return &n;
        return nullptr;
    };

    // --- 设置样式 ---
    glColor4f(0.3f, 0.3f, 0.3f, 0.4f);

    // --- 开启多边形偏移 ---
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    for (const auto& face : faces) {
        // 使用 GL_POLYGON 绘制多边形
        glBegin(GL_POLYGON);

        // 遍历构成这个面的所有点索引
        for (int nodeId : face.nodeIndices) {
            const Node* n = findNode(nodeId);
            if (n) {
                glVertex3f(n->x, n->y, n->z);
            }
        }
        glEnd();
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
}

void Plotter3D::drawFaceInfo()
{
    if (!m_data) return;
    const auto& faces = m_data->getFaces();

    QPainter painter(this);
    painter.setPen(Qt::yellow); // 用黄色显示面积
    painter.setFont(QFont("Arial", 10));

    QRect viewport(0, 0, width(), height());

    for (const auto& face : faces) {
        // 使用库算出来的中心点
        QVector3D center(face.centerX, face.centerY, face.centerZ);

        // 投影到屏幕
        QVector3D screenPos = center.project(m_modelView, m_projection, viewport);

        if (screenPos.z() > 1.0f) continue; // 裁切

        double x = screenPos.x();
        double y = height() - screenPos.y();

        // 绘制面积文字，保留2位小数
        QString text = QString("Area: %1").arg(face.area, 0, 'f', 2);

        // 文字居中处理
        int textWidth = painter.fontMetrics().horizontalAdvance(text);
        painter.drawText(x - textWidth / 2, y, text);
    }
}
