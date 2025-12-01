#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 初始化数据
    m_meshData = new MeshData();

    // 2. 初始化模型，并把数据传给它
    // 点表格
    m_nodeModel = new NodeTableModel(m_meshData, this);
    ui->tableNodes->setModel(m_nodeModel);
    // 线表格
    m_elemModel = new ElementTableModel(m_meshData, this);
    ui->tableElements->setModel(m_elemModel);

    // 美化一下表格列宽
    ui->tableElements->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableNodes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->view3D->setMeshData(m_meshData);


    // 2. 选中模式：支持多选 (ExtendedSelection: 按Ctrl多选，按Shift连选)
    // 如果只想单选，用 QAbstractItemView::SingleSelection
    ui->tableNodes->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableNodes->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableElements->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableElements->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // --- 2. 连接信号 ---
    // 监听线表格的选择变化
    connect(ui->tableElements->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onElemTableSelectionChanged);

    connect(ui->tableNodes->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onTableSelectionChanged);
    connect(ui->view3D, &Plotter3D::nodeClicked, this, [=](int nodeId){
        // 1. 如果不是连线模式，就执行“同步选中”逻辑
        if (!m_isLineMode && !m_isArcMode) {
            // 遍历数据，找到 ID 对应的行号 (Row Index)
            // 注意：行号不一定等于 ID，因为删除后 ID 会断号
            const auto& nodes = m_meshData->getNodes();
            int targetRow = -1;
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (nodes[i].id == nodeId) {
                    targetRow = static_cast<int>(i);
                    break;
                }
            }

            if (targetRow != -1) {
                // 在表格中选中这一行
                ui->tableNodes->selectRow(targetRow);

                // 这一步很重要：手动触发一下高亮逻辑 (虽然selectRow会触发信号，但双保险)
                // 或者直接依赖我们之前写的 onTableSelectionChanged 槽函数
            }
            return;
        }

        // --- 连线逻辑 ---
        if (m_isLineMode){
            if (m_startNodeId == -1) {
                // 阶段 1：选中了第一个点
                m_startNodeId = nodeId;

                // 高亮它，提示用户
                std::vector<int> ids = {nodeId};
                ui->view3D->setHighlightIndices(ids);

                ui->statusbar->showMessage(tr("Start point selected. Click another point."), 0);
            }
            else {
                // 阶段 2：选中了第二个点，连线！
                if (m_startNodeId != nodeId) {
                    m_meshData->addLine(m_startNodeId, nodeId);
                    m_elemModel->refresh();

                    // 连完后，清空状态
                    m_startNodeId = -1;
                    ui->view3D->setHighlightIndices({}); // 取消高亮
                    ui->view3D->update(); // 重绘

                    ui->statusbar->showMessage(tr("Line created!"), 2000);
                }
            }
        }
        if (m_isArcMode) {
            if (m_arcNode1 == -1) {
                // 阶段 1：选中起点
                m_arcNode1 = nodeId;
                ui->view3D->setHighlightIndices({nodeId});
                ui->statusbar->showMessage(tr("Arc: Start point selected. Select Middle point."));
            }
            else if (m_arcNode2 == -1) {
                // 阶段 2：选中中间点
                if (nodeId == m_arcNode1) return; // 不能选同一个点
                m_arcNode2 = nodeId;
                ui->view3D->setHighlightIndices({m_arcNode1, m_arcNode2});
                ui->statusbar->showMessage(tr("Arc: Middle point selected. Select End point."));
            }
            else {
                // 阶段 3：选中终点 -> 生成！
                if (nodeId == m_arcNode1 || nodeId == m_arcNode2) return;

                // 1. 获取中间点的坐标 (因为 MeshData::addArc 需要坐标)
                const auto& nodes = m_meshData->getNodes();
                double mx=0, my=0, mz=0;
                for(const auto& n : nodes) {
                    if(n.id == m_arcNode2) { mx=n.x; my=n.y; mz=n.z; break; }
                }

                // 2. 添加数据 (起点ID, 终点ID, 中间点坐标)
                m_meshData->addArc(m_arcNode1, nodeId, mx, my, mz);

                // 3. 刷新视图和表格
                m_elemModel->refresh();
                ui->view3D->update();

                // 4. 重置状态
                m_arcNode1 = -1;
                m_arcNode2 = -1;
                ui->view3D->setHighlightIndices({});
                ui->statusbar->showMessage(tr("Arc created! Select Start point for next arc."));
            }
            return;
        }
    });
    connect(ui->view3D, &Plotter3D::elementClicked, this, [=](int elemId){
        // 1. 如果正在连线模式，不要选线，容易误触
        if (m_isLineMode) return;

        // 2. 切换到底部的 "Elements" 标签页，让用户知道现在选中了线
        ui->tabWidget->setCurrentIndex(1);

        // 3. 在线表格中找到这一行并选中
        const auto& elements = m_meshData->getElements();
        int targetRow = -1;
        for (size_t i = 0; i < elements.size(); ++i) {
            if (elements[i].id == elemId) {
                targetRow = static_cast<int>(i);
                break;
            }
        }
        if (targetRow != -1) {
            ui->tableElements->selectRow(targetRow);
            // (可选) 顺便让 Node 表格取消选择，避免混淆
            ui->tableNodes->clearSelection();
        }
    });
    connect(ui->tableElements->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onElemTableSelectionChanged);

    // 菜单按钮
    connect(ui->actionShowFaceInfo, &QAction::toggled,
            ui->view3D, &Plotter3D::setShowFaceInfo);
    // connect(ui->actionClear, )
}

MainWindow::~MainWindow()
{
    delete m_meshData; // 记得手动删除非QObject对象
    delete ui;
}
void MainWindow::on_actionImport_triggered()
{
    // 1. 获取文件路径
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Import Mesh"), "", tr("Text Files (*.txt);;All Files (*)"));

    if (fileName.isEmpty()) return;

    // 2. 提示是否覆盖当前数据（如果当前有数据）
    if (!m_meshData->getNodes().empty()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Import",
                                      "Importing will clear current data. Continue?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) return;
    }

    // 3. 打开文件
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open file!");
        return;
    }

    // 4. 清空当前场景
    // (确保 MeshData::clear() 同时也重置了 m_nextNodeId 和 m_nextElementId 为 0)
    m_meshData->clearData();

    QTextStream in(&file);
    QString line;
    int mode = 0; // 0=None, 1=Nodes, 2=Elements

    // 开始解析
    while (!in.atEnd()) {
        line = in.readLine().trimmed();

        // 跳过空行和注释
        if (line.isEmpty() || line.startsWith("#")) continue;

        // 检测头部关键字
        if (line.startsWith("NODES")) {
            mode = 1;
            continue; //这一行是计数，跳过，直接读下一行数据
        }
        else if (line.startsWith("EDGES") || line.startsWith("ELEMENTS")) {
            mode = 2;
            continue;
        }

        // 按空格分割数据
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);

        if (mode == 1) { // 解析节点
            // 格式: ID X Y Z
            // 注意：因为我们刚清空了数据，addNode 会自动产生 ID。
            // 假设文件里的 ID 是顺序排列的(0,1,2...)，那么 addNode 产生的 ID 会和文件一致。
            if (parts.size() >= 4) {
                double x = parts[1].toDouble();
                double y = parts[2].toDouble();
                double z = parts[3].toDouble();
                m_meshData->addNode(x, y, z);
            }
        }
        else if (mode == 2) { // 解析单元
            // 格式: ID TYPE START END [midX midY midZ]
            // Line: 0 0 0 8 0 0 0
            // Arc:  20 1 9 10 2.5 0 2
            if (parts.size() >= 4) {
                // parts[0] 是 ID (我们忽略它，让 addLine 自动生成)
                int type = parts[1].toInt();
                int startId = parts[2].toInt();
                int endId = parts[3].toInt();

                if (type == 0) {
                    // 直线
                    m_meshData->addLine(startId, endId);
                }
                else if (type == 1 && parts.size() >= 7) {
                    // 弧线 (需要读取 mid 坐标)
                    double mx = parts[4].toDouble();
                    double my = parts[5].toDouble();
                    double mz = parts[6].toDouble();
                    m_meshData->addArc(startId, endId, mx, my, mz);
                }
            }
        }
    }

    file.close();

    // 5. 刷新 UI
    m_nodeModel->refresh();
    m_elemModel->refresh();

    // 清除任何可能的高亮残留
    ui->view3D->setHighlightIndices({});
    ui->view3D->setHighlightElementIndices({});
    ui->view3D->update();

    ui->statusbar->showMessage("Data imported successfully!", 3000);

}
void MainWindow::on_actionExport_triggered()
{
    // 1. 打开文件保存对话框
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Export Mesh"), "", tr("Text Files (*.txt);;All Files (*)"));

    if (fileName.isEmpty()) return; // 用户取消了

    // 2. 打开文件准备写入
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot write to file!");
        return;
    }

    QTextStream out(&file);

    // 3. 写入点数据
    // 格式：NODE [ID] [X] [Y] [Z]
    const auto& nodes = m_meshData->getNodes();
    out << "# Mesh Data Export\n";
    out << "# Format: NODE id x y z\n";
    out << "NODES " << nodes.size() << "\n";

    for (const auto& node : nodes) {
        out << node.id << " "
            << node.x << " " << node.y << " " << node.z << "\n";
    }

    // 4. 写入边数据
    // 格式：ELEM [ID] [TYPE] [START] [END]
    const auto& elements = m_meshData->getElements();
    out << "\n# Format: ELEM id type(0=Line,1=Arc) start_node end_node midX midY midZ\n";
    out << "EDGES " << elements.size() << "\n";

    for (const auto& elem : elements) {
        out << elem.id << " "
            << elem.type << " "
            << elem.startNodeId << " " <<
            elem.endNodeId  << " " <<
            elem.midX << " " <<
            elem.midY << " " <<
            elem.midZ << "\n";
    }

    file.close();
    ui->statusbar->showMessage("Data exported successfully!", 3000);
}
// 当点击“添加点”按钮时
void MainWindow::on_btnAddPoint_clicked()
{
    // 1. 获取输入框的值
    double x = ui->spinX->value();
    double y = ui->spinY->value();
    double z = ui->spinZ->value();

    // 2. 存入数据层
    m_meshData->addNode(x, y, z);

    // 3. 通知表格更新
    m_nodeModel->refresh();
    ui->view3D->update();

    // 4. (可选) 状态栏提示
    ui->statusbar->showMessage("Node Added!", 2000);
}
void MainWindow::on_btnMesh_clicked()
{
    // 1. 调用算法
    m_meshData->generateFaces();

    // 2. 获取结果数量进行反馈
    int faceCount = m_meshData->getFaces().size();

    if (faceCount > 0) {
        ui->statusbar->showMessage(QString("Success! Generated %1 faces.").arg(faceCount), 5000);

        // TODO: 下一步就是去 Plotter3D 里把 m_faces 画出来
        ui->view3D->update();
    } else {
        ui->statusbar->showMessage("No faces found. Check your closed loops.", 3000);
    }
}
void MainWindow::on_btnDeletePoint_clicked()
{
    int currentTab = ui->tabWidget->currentIndex();

    // ==========================================
    // 情况 A：在“节点列表” (Tab 0) 执行删除
    // ==========================================
    if (currentTab == 0) {
        QItemSelectionModel *select = ui->tableNodes->selectionModel();

        if (!select->hasSelection()) {
            ui->statusbar->showMessage("Please select a node in the table first!", 2000);
            return;
        }

        QModelIndexList selectedRows = select->selectedRows();

        // 1. 【关键】必须倒序排列！(从下往上删)
        // 否则删了第2行，第3行就变成第2行了，后面的索引会全乱
        std::sort(selectedRows.begin(), selectedRows.end(), [](const QModelIndex &a, const QModelIndex &b){
            return a.row() > b.row(); // 降序：行号大的排前面
        });

        // 2. 循环删除
        for (const QModelIndex &idx : selectedRows) {
            int row = idx.row();

            // 获取要删除的点的 ID (此时 row 还是准确的)
            int nodeId = ui->tableNodes->model()->data(ui->tableNodes->model()->index(row, 0)).toInt();

            // A. 先删连接的线 (内部调用 removeElementAtIndex，保证线ID也连续)
            m_meshData->removeElementsConnectedTo(nodeId);

            // B. 再删点 (MeshData 内部会把 >nodeId 的所有点 ID 减 1)
            m_nodeModel->removeRow(row);
        }

        // 3. 【核心修正】强制刷新所有表格
        // 此时数据层 ID 已经变了(例如点4变成了点3)，必须告诉表格重新读取数据
        m_nodeModel->refresh();
        m_elemModel->refresh(); // 线的数据(连接关系)也变了，必须刷新

        // 4. 清理 3D 视图
        ui->view3D->setHighlightIndices({}); // 清空高亮，防止错位
        ui->view3D->update();

        ui->statusbar->showMessage("Nodes and connected lines deleted (IDs compacted).", 2000);
    }
    // ==========================================
    // 情况 B：在“单元列表” (Tab 1) 执行删除
    // ==========================================
    else if (currentTab == 1) {
        QItemSelectionModel *select = ui->tableElements->selectionModel();
        if (!select->hasSelection()) return;

        QModelIndexList selectedRows = select->selectedRows();

        // 1. 倒序排列
        std::sort(selectedRows.begin(), selectedRows.end(), [](const QModelIndex &a, const QModelIndex &b){
            return a.row() > b.row();
        });

        // 2. 循环删除
        for (const QModelIndex &idx : selectedRows) {
            // 调用 Element Model 的删除 (MeshData 内部会处理 ID 连续)
            m_elemModel->removeRow(idx.row());
        }

        // 3. 【核心修正】强制刷新表格
        // 同样的道理，删了线3，线4变成了线3，需要刷新显示
        m_elemModel->refresh();

        // 4. 清理 3D 视图
        ui->view3D->setHighlightElementIndices({});
        ui->view3D->update();

        ui->statusbar->showMessage("Edges deleted (IDs compacted).", 2000);
    }
}

void MainWindow::onTableSelectionChanged()
{
    // 1. 获取所有选中行
    QModelIndexList selectedRows = ui->tableNodes->selectionModel()->selectedRows();

    // 2. 提取行号（索引）存入 vector
    std::vector<int> selectedIndices;
    for (const QModelIndex& idx : selectedRows) {
        selectedIndices.push_back(idx.row());
    }

    // 3. 把这些索引传给 3D 视图
    ui->view3D->setHighlightIndices(selectedIndices);
    if (!selectedIndices.empty()) {
        // blockSignals 是为了防止死循环 (A清除B -> B触发信号清除A -> A又清除B...)
        // 虽然在这里逻辑上不会死循环，但是个好习惯
        ui->tableElements->selectionModel()->blockSignals(true);
        ui->tableElements->clearSelection();
        ui->tableElements->selectionModel()->blockSignals(false);

        // 同时手动清除 3D 里的线高亮
        ui->view3D->setHighlightElementIndices({});
    }

    // (调试用) 可以在状态栏显示选中了几个
    ui->statusbar->showMessage(QString("Selected %1 nodes").arg(selectedIndices.size()));
}

void MainWindow::on_btnToggleLine_clicked()
{
    // 如果按钮是 Checkable 的，可以直接用 isChecked()
    // 假设你在 UI 里把 btnToggleLine 设为了 Checkable
    m_isLineMode = ui->btnToggleLine->isChecked();
    if (m_isLineMode) {
        ui->btnToggleArc->setChecked(false);
        m_isArcMode = false; // 关掉弧线
        // ...
    }

    // 重置状态
    m_startNodeId = -1;
    ui->view3D->setHighlightIndices({});

    if (m_isLineMode) {
        ui->statusbar->showMessage("Line Mode: ON. Click two points to connect.");
    } else {
        ui->statusbar->showMessage("Line Mode: OFF.");
    }
}

// 1. 弧线模式按钮槽函数
void MainWindow::on_btnToggleArc_clicked()
{
    m_isArcMode = ui->btnToggleArc->isChecked();

    // 互斥逻辑：如果开了弧线，就关掉连线；反之亦然
    if (m_isArcMode) {
        ui->btnToggleLine->setChecked(false);
        m_isLineMode = false;

        ui->statusbar->showMessage("Arc Mode: Select Start Point.");
    } else {
        ui->statusbar->showMessage("Arc Mode: OFF.");
    }

    // 重置所有临时状态
    m_startNodeId = -1; // 连线用的
    m_arcNode1 = -1;    // 弧线用的
    m_arcNode2 = -1;
    ui->view3D->setHighlightIndices({});
}

void MainWindow::onElemTableSelectionChanged()
{
    // 1. 获取线表格中被选中的所有行
    QModelIndexList selectedRows = ui->tableElements->selectionModel()->selectedRows();

    // 2. 提取行号（也就是线的 Index）
    std::vector<int> selectedIndices;
    for (const QModelIndex& idx : selectedRows) {
        selectedIndices.push_back(idx.row());
    }

    // 3. 告诉 3D 视图高亮这些线
    ui->view3D->setHighlightElementIndices(selectedIndices);

    // 4. (优化体验) 如果选中了线，最好把点的选中状态清除掉，避免视觉混乱
    if (!selectedIndices.empty()) {
        // 这行代码会触发 onTableSelectionChanged，从而自动清除 3D 点的高亮
        ui->tableNodes->clearSelection();
    }
}

void MainWindow::on_actionClear_triggered(){
    qDebug("clear data");
    // 2. 清空底层数据
    m_meshData->clearData();

    // 3. 刷新两个表格
    // 我们的 Model 里的 refresh() 调用了 beginResetModel，所以表格会瞬间变空
    m_nodeModel->refresh();
    m_elemModel->refresh();

    // 4. 重置 3D 视图状态
    ui->view3D->setHighlightIndices({});
    ui->view3D->setHighlightElementIndices({});
    ui->view3D->update(); // 重绘

    // 5. 重置主窗口内部逻辑变量
    m_startNodeId = -1;
    // 如果你在连线模式，可以选择保持或退出，这里假设保持
    ui->statusbar->showMessage("Canvas cleared.", 2000);

}
