#ifndef ELEMENTTABLEMODEL_H
#define ELEMENTTABLEMODEL_H

#include <QAbstractTableModel>
#include "meshdata.h"

class ElementTableModel : public QAbstractTableModel
{
public:
    explicit ElementTableModel(MeshData *data, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void refresh(); // 刷新数据
    void removeRow(int row);

private:
    MeshData *m_data;
};

#endif // ELEMENTTABLEMODEL_H
