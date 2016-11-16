#include <QtCore/QVariant>
#include <QtCore/QModelIndex>
#include <QtCore/QVariant>
#include <QtCore/QHash>
#include <QtCore/QByteArray>
#include <QtCore/QStringList>

#include "stringtablemodel.h"

StringTableModel::StringTableModel(QString emptyCell) : m_emptyCell(emptyCell)
{}

StringTableModel::~StringTableModel()
{
    for(int i=0; i<m_list.length(); i++)
        delete m_list[i];
}

int StringTableModel::rowCount(const QModelIndex &parent) const
{
    return m_list.length();
}

int StringTableModel::columnCount(const QModelIndex &parent) const
{
    return m_roleList.length();
}

QVariant StringTableModel::data(const QModelIndex &index, int role) const
{
    if(m_list[index.row()]->length()<=role)
    {
        return QVariant(m_emptyCell);
    }
    
    return QVariant(m_list[index.row()]->at(role));
}

QHash<int, QByteArray> StringTableModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    for(int i=0; i<m_roleList.length(); i++)
        roles[i]=m_roleList[i].toUtf8().constData();

    return roles;
}

QStringList& StringTableModel::newRow()
{
    QStringList* l = new QStringList();
    
    m_list.append(l);
    
    return *l;        
}

QStringList& StringTableModel::lastRow()
{
    return *m_list.constLast();
}

QStringList& StringTableModel::roles()
{
    return m_roleList;
}
