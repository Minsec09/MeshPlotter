#ifndef NODETABLEMODEL_H
#define NODETABLEMODEL_H

#include <QAbstractTableModel>
#include "meshdata.h" // 包含数据头文件

class NodeTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    void removeRow(int row);
    // 构造函数需要传入 MeshData 的指针，这样Model才能读到数据
    explicit NodeTableModel(MeshData *data, QObject *parent = nullptr);

    // 必须实现的三个虚函数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // 设置表头（显示 ID, X, Y, Z）
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 自定义函数：通知表格刷新
    void refresh();

private:
    MeshData *m_data; // 持有数据的指针
};

#endif // NODETABLEMODEL_H
