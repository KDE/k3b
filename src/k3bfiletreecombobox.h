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

#ifndef _K3B_FILETREE_COMBOBOX_H_
#define _K3B_FILETREE_COMBOBOX_H_

#include <kcombobox.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QPixmap>

namespace K3b {
    class FileTreeView;
}
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

namespace Device {
    class Device;
}

namespace K3b {
class FileTreeComboBox : public KComboBox
{
    Q_OBJECT

public:
    FileTreeComboBox( QWidget* parent = 0 );
    ~FileTreeComboBox();

public Q_SLOTS:
    void setDevice( Device::Device* );
    void setUrl( const KUrl& url );
    void slotGoUrl();

 Q_SIGNALS:
    void activated( const KUrl& url );
    void activated( Device::Device* dev );

private Q_SLOTS:
    void slotDeviceExecuted( Device::Device* );
    void slotUrlExecuted( const KUrl& url );

private:
    class Private;
    Private* d;
    FileTreeView* m_fileTreeView;
};
}

#endif
