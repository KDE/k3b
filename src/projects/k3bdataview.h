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


#ifndef K3BDATAVIEW_H
#define K3BDATAVIEW_H

#include <k3bview.h>

class K3bDataDoc;
class K3bDataItem;
class K3bFileItem;
class K3bDirItem;
class K3bDataDirTreeView;
class K3bDataFileView;
class QLineEdit;


namespace KIO {
    class Job;
}
namespace K3bDevice {
    class Device;
}



/**
 *@author Sebastian Trueg
 */
class K3bDataView : public K3bView
{
    Q_OBJECT

public:
    K3bDataView(K3bDataDoc* doc, QWidget *parent=0);
    virtual ~K3bDataView();
	
    K3bDirItem* currentDir() const;

public slots:
    void slotBurn();
    void importSession();
    void clearImportedSession();
    void editBootImages();

    void slotDocChanged();

    void addUrls( const KUrl::List& );

protected:
    K3bDataDirTreeView* m_dataDirTree;
    K3bDataFileView* m_dataFileView;
    QLineEdit* m_volumeIDEdit;

    virtual K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );
		
private:
    K3bDataDoc* m_doc;

    // used for mounting when importing old session
    K3bDevice::Device* m_device;
};


#endif
