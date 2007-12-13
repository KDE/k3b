/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BFILETREEVIEW_H
#define K3BFILETREEVIEW_H


#include <k3filetreeview.h>
//Added by qt3to4:
#include <QPixmap>
#include <QDragEnterEvent>
#include <QDropEvent>

class KFileTreeBranch;
class KActionCollection;
class KActionMenu;
class QPoint;
class QDropEvent;
class QDragEnterEvent;

namespace K3bDevice {
    class Device;
    class DeviceManager;
}

namespace KIO {
    class Job;
}


class K3bDeviceBranch : public KFileTreeBranch
{
    Q_OBJECT

public:
    K3bDeviceBranch( K3FileTreeView*, K3bDevice::Device* dev, K3FileTreeViewItem* item = 0 );

    K3bDevice::Device* device() const { return m_device; }

    /**
     * Adds or removes the blockdevicename from the branch name
     */
    void showBlockDeviceName( bool b );

public Q_SLOTS:
    void setCurrent( bool );

    bool populate( const KUrl& url,  K3FileTreeViewItem *currItem );

private Q_SLOTS:
    void slotMediumChanged( K3bDevice::Device* );

private:
    void updateLabel();

    K3bDevice::Device* m_device;
    bool m_showBlockDeviceName;
};


class K3bFileTreeBranch : public KFileTreeBranch
{
public:
    K3bFileTreeBranch( K3FileTreeView*,
                       const KUrl& url,
                       const QString& name,
                       const QPixmap& pix,
                       bool showHidden,
                       K3FileTreeViewItem& item );
};


class K3bDeviceBranchViewItem : public K3FileTreeViewItem
{
public:
    K3bDeviceBranchViewItem( K3FileTreeViewItem*, K3bDevice::Device*, K3bDeviceBranch* );
    K3bDeviceBranchViewItem( K3FileTreeView*, K3bDevice::Device*, K3bDeviceBranch* );

    QString key( int column, bool ascending ) const;

    void setCurrent( bool );

    void paintCell( QPainter* p, const QColorGroup& cg, int col, int width, int align );

    int widthHint() const;

private:
    bool m_bCurrent;

    K3bDevice::Device* m_device;
};


class K3bFileTreeViewItem : public K3FileTreeViewItem
{
public:
    K3bFileTreeViewItem( K3FileTreeViewItem*, KFileItem&, KFileTreeBranch* );
    K3bFileTreeViewItem( K3FileTreeView *, KFileItem&, KFileTreeBranch* );

    QString key( int column, bool ascending ) const;
};


/**
 *@author Sebastian Trueg
 */
class K3bFileTreeView : public K3FileTreeView
{
    Q_OBJECT

public: 
    K3bFileTreeView( QWidget *parent = 0 );
    ~K3bFileTreeView();


    virtual KFileTreeBranch* addBranch( KFileTreeBranch* );
    virtual KFileTreeBranch* addBranch( const KUrl& url, const QString& name, const QPixmap& , bool showHidden = false );

    K3bDeviceBranch* branch( K3bDevice::Device* dev );

    /**
     * returns 0 if no device is selected 
     */
    K3bDevice::Device* selectedDevice() const;

    /** 
     * returnes an empty url if no url is selected
     */
    KUrl selectedUrl() const;

public Q_SLOTS:
    /**
     * adds home and root dir branch
     */
    void addDefaultBranches();
    void addCdDeviceBranches( K3bDevice::DeviceManager* );
    void addDeviceBranch( K3bDevice::Device* dev );

    /**
     * Make dev the current device. This does not mean that the device entry
     * will be highlighted but marked otherwise since this means that it is the
     * current device in the application and not the treeview.
     */
    void setCurrentDevice( K3bDevice::Device* dev );

    /**
     * his will highlight the device and also make it the current device.
     */
    void setSelectedDevice( K3bDevice::Device* dev );

    void followUrl( const KUrl& url );
    void setTreeDirOnlyMode( bool b );
    void enablePopupMenu( bool b ) { m_menuEnabled = b; }

    /**
     * @reimplemented
     */
    virtual void clear();

    void updateMinimumWidth();

signals:
    void urlExecuted( const KUrl& url );
    void deviceExecuted( K3bDevice::Device* dev );

    /** only gets emitted if the menu is disabled */
    void contextMenu( K3bDevice::Device*, const QPoint& );
    /** only gets emitted if the menu is disabled */
    void contextMenu( const KUrl& url, const QPoint& );
  
    private slots:
    void slotItemExecuted( Q3ListViewItem* item );
    void slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& );
    void slotSettingsChangedK3b(int category);
    void slotMouseButtonClickedK3b( int btn, Q3ListViewItem *item, const QPoint &pos, int c );

private:
    void initActions();

    class Private;
    Private* d;

    bool m_dirOnlyMode;
    KActionCollection* m_actionCollection;
    KActionMenu* m_devicePopupMenu;
    KActionMenu* m_urlPopupMenu;
    bool m_menuEnabled;
};

#endif
