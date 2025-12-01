#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "meshdata.h"       // 引入
#include "nodetablemodel.h" // 引入
#include "elementtablemodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 对应界面上“添加点”按钮的槽函数
    void on_btnAddPoint_clicked();
    void on_btnDeletePoint_clicked();
    void onTableSelectionChanged();
    void onElemTableSelectionChanged();
    void on_btnToggleLine_clicked();
    void on_actionClear_triggered(); // 清屏
    void on_actionExport_triggered();
    void on_btnToggleArc_clicked();
    void on_actionImport_triggered();
    void on_btnMesh_clicked();

private:
    Ui::MainWindow *ui;

    // 核心成员变量
    MeshData *m_meshData;         // 数据中心
    NodeTableModel *m_nodeModel;  // 点表格
    ElementTableModel *m_elemModel; // 线表格
    bool m_isLineMode = false; // 是否处于连线模式
    int m_startNodeId = -1;    // 连线的第一点 ID

    bool m_isArcMode = false;

    // 记录弧线的临时点 ID
    int m_arcNode1 = -1; // 起点
    int m_arcNode2 = -1; // 中间点
};
#endif // MAINWINDOW_H
