#include <QtCore/QVariant>
#include <QtCore/QByteArray>
#include <QtCore/QStringList>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QModelIndex>

#include "stringgridmodel.h"

StringGridModel::StringGridModel()
{}

StringGridModel::~StringGridModel()
{}

StringGridModel& StringGridModel::operator<<(const GridItem& item)
{
    if(item.m_colspan>0)
    {
        GridItem gi(item);
        list << gi;
    }
    return *this;
}

StringGridModel& StringGridModel::operator<<(const char* str)
{
    GridItem gi(QString(str), 1);
    list << gi;
    return *this;
}

StringGridModel& StringGridModel::operator<<(const QString& str)
{
    GridItem gi(str, 1);
    list << gi;
    return *this;
}

int StringGridModel::rowCount(const QModelIndex& parent) const
{
    return list.length();
}

int StringGridModel::getRow(int columnCount, int index)
{

    int cell=0;

    for(int i=0; i<=index; i++)
        cell+=list[i].m_colspan;

    return (cell-1)/columnCount;
}

bool StringGridModel::isRowEvenNumbered(int columnCount, int index)
{
    return getRow(columnCount,index)%2==0;
}

int StringGridModel::getRowCount(int columnCount)
{
    int cell=0, index = list.length()-1;

    for(int i=0; i<=index; i++)
        cell+=list[i].m_colspan;

    return 1+(cell-1)/columnCount;
}

QVariant StringGridModel::data(const QModelIndex& index, int role) const
{
    switch(role)
    {
        case 0:
            return QVariant(list[index.row()].m_text);
        case 1:
            return QVariant::fromValue(list[index.row()].m_colspan);
        default:
            return QVariant(QString(""));
    }
}

QHash<int, QByteArray> StringGridModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[0]="itemText";
    roles[1]="colspan";

    return roles;
}
