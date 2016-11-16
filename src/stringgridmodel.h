#ifndef STRINGGRIDMODEL_H
#define STRINGGRIDMODEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QModelIndex>

class QVariant;
class QByteArray;
class QStringList;

class GridItem : public QObject
{
    Q_OBJECT
public:
    explicit GridItem(QString text, int colspan = 1, QObject *parent = 0) : QObject(parent), m_text(text), m_colspan(colspan)
    {}

    GridItem(): GridItem("", 0)
    {}

    GridItem(const GridItem& other): GridItem(other.m_text, other.m_colspan)
    {}

    const QString m_text;
    const int m_colspan;
};
Q_DECLARE_METATYPE( GridItem )


class StringGridModel : public QAbstractListModel
{
    Q_OBJECT
public:
    StringGridModel();
    ~StringGridModel();

    StringGridModel& operator<<(const GridItem& item);
    StringGridModel& operator<<(const char* str);
    StringGridModel& operator<<(const QString& str);

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    Q_INVOKABLE int getRow(int columnCount, int index);
    Q_INVOKABLE bool isRowEvenNumbered(int columnCount, int index);
    Q_INVOKABLE int getRowCount(int columnCount);

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    QList<GridItem> list;
};

#endif // STRINGGRIDMODEL_H
