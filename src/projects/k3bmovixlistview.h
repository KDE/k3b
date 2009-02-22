/* 
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef _K3B_MOVIX_LISTVIEW_H_
#define _K3B_MOVIX_LISTVIEW_H_

#include <k3blistview.h>
#include <kfileitem.h>

#include <qmap.h>
//Added by qt3to4:
#include <QDropEvent>


namespace K3b {
    class MovixDoc;
}
namespace K3b {
    class MovixFileItem;
}
namespace K3b {
    class FileItem;
}


namespace K3b {
class MovixListViewItem : public ListViewItem
{
public:
    MovixListViewItem( MovixDoc* doc, MovixFileItem*, Q3ListView* parent, Q3ListViewItem* after );
    MovixListViewItem( MovixDoc* doc, MovixFileItem*, Q3ListViewItem* parent );
    ~MovixListViewItem();

    MovixFileItem* fileItem() const { return m_fileItem; }
    MovixDoc* doc() const { return m_doc; }

    virtual bool isMovixFileItem() const { return true; }

private:
    MovixDoc* m_doc;
    MovixFileItem* m_fileItem;
};
}


namespace K3b {
class MovixFileViewItem : public MovixListViewItem, public KFileItem
{
public:
    MovixFileViewItem( MovixDoc* doc, MovixFileItem*, Q3ListView* parent, Q3ListViewItem* );

    QString text( int ) const;
    void setText(int col, const QString& text );

    /** always sort according to the playlist order */
    QString key( int, bool ) const;
};
}

namespace K3b {
class MovixSubTitleViewItem : public MovixListViewItem, public KFileItem
{
public:
    MovixSubTitleViewItem( MovixDoc*, MovixFileItem* item, MovixListViewItem* parent );
    ~MovixSubTitleViewItem();

    QString text( int ) const;

    bool isMovixFileItem() const { return false; }
};
}


namespace K3b {
class MovixListView : public ListView
{
    Q_OBJECT

public:
    MovixListView( MovixDoc* doc, QWidget* parent = 0 );
    ~MovixListView();

    Q3DragObject* dragObject();

protected:
    bool acceptDrag(QDropEvent* e) const;

    private Q_SLOTS:
    void slotNewFileItems();
    void slotFileItemRemoved( MovixFileItem* );
    void slotSubTitleItemRemoved( MovixFileItem* );
    void slotDropped( K3ListView*, QDropEvent* e, Q3ListViewItem* after );
    void slotChanged();

private:
    MovixDoc* m_doc;

    QMap<FileItem*, MovixFileViewItem*> m_itemMap;
};
}

#endif
