#ifndef STRINGTABLEMODEL_H
#define STRINGTABLEMODEL_H

#include <QtCore/QString>
#include <QtCore/QAbstractTableModel>
#include <QtCore/QHash>
#include <QtCore/QList>

class QModelIndex;
class QVariant;
class QByteArray;
class QStringList;

class StringTableModel : public QAbstractTableModel
{
public:
    StringTableModel(QString emptyCell = QString(""));
    ~StringTableModel();
    
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    QStringList& newRow();
    QStringList& lastRow();
    QStringList& roles();

private:
    QString m_emptyCell;
    QStringList m_roleList;
    QList<QStringList*> m_list;
};

#endif // STRINGTABLEMODEL_H
