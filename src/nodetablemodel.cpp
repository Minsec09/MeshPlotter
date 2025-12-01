#include "nodetablemodel.h"

NodeTableModel::NodeTableModel(MeshData *data, QObject *parent)
    : QAbstractTableModel(parent), m_data(data)
{
}
void NodeTableModel::removeRow(int row)
{
    beginRemoveRows(QModelIndex(), row, row);

    // 2. 真正的删除数据
    m_data->removeNodeAtIndex(row);

    // 3. 通知 View：删除结束，请更新显示
    endRemoveRows();
}
int NodeTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    // 行数 = 点的个数
    return static_cast<int>(m_data->getNodes().size());
}

int NodeTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    // 我们显示4列：ID, X, Y, Z
    return 4;
}

QVariant NodeTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    // 获取当前行对应的点
    const Node& node = m_data->getNodes()[index.row()];

    // 根据列号决定显示什么
    switch (index.column()) {
    case 0: return node.id;
    case 1: return QString::number(node.x, 'f', 2); // 保留2位小数
    case 2: return QString::number(node.y, 'f', 2);
    case 3: return QString::number(node.z, 'f', 2);
    default: return QVariant();
    }
}

QVariant NodeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case 0: return "ID";
    case 1: return "X";
    case 2: return "Y";
    case 3: return "Z";
    default: return QVariant();
    }
}

void NodeTableModel::refresh()
{
    // 告诉View：数据要重置了，重新读取一下
    beginResetModel();
    endResetModel();
}
