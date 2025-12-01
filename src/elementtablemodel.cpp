#include "elementtablemodel.h"

ElementTableModel::ElementTableModel(MeshData *data, QObject *parent)
    : QAbstractTableModel(parent), m_data(data)
{
}

int ElementTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_data->getElements().size());
}

int ElementTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return 7; // ID, Type, Start, End, midX, midY, midZ
}

QVariant ElementTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const auto& elem = m_data->getElements()[index.row()];

    switch (index.column()) {
    case 0: return elem.id;
    case 1: return (elem.type == TYPE_LINE) ? "Line" : "Arc";
    case 2: return elem.startNodeId;
    case 3: return elem.endNodeId;
    case 4: return elem.midX;
    case 5: return elem.midY;
    case 6: return elem.midZ;
    default: return QVariant();
    }
}

QVariant ElementTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case 0: return "ID";
    case 1: return "Type";
    case 2: return "Start Node";
    case 3: return "End Node";
    case 4: return "midX";
    case 5: return "midY";
    case 6: return "midZ";
    default: return QVariant();
    }
}

void ElementTableModel::refresh()
{
    beginResetModel();
    endResetModel();
}
void ElementTableModel::removeRow(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_data->removeElementAtIndex(row); // 调用数据层的删除
    endRemoveRows();
}
