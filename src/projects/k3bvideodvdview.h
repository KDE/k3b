/* 
 *
 * Copyright (C) 2005-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_VIDEO_DVDVIEW_H_
#define _K3B_VIDEO_DVDVIEW_H_

#include <k3bstandardview.h>

class K3bVideoDvdDoc;
//class K3bDataDirTreeView;
//class K3bDataFileView;

namespace K3b {
	class DataProjectModel;
}

class K3bVideoDvdView : public K3bStandardView
{
    Q_OBJECT

public:
    K3bVideoDvdView( K3bVideoDvdDoc* doc, QWidget *parent = 0 );
    ~K3bVideoDvdView();

    void addUrls( const KUrl::List& );

protected:
    virtual K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

    void init();

private:
    K3bVideoDvdDoc* m_doc;
    //K3bDataDirTreeView* m_dataDirTree;
    //K3bDataFileView* m_dataFileView;
	K3b::DataProjectModel* m_model;
};

#endif
