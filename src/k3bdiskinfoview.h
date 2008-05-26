/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef K3BDISKINFOVIEW_H
#define K3BDISKINFOVIEW_H

#include "k3bmediacontentsview.h"

class QTextBrowser;

class K3bDiskInfoView : public K3bMediaContentsView
{
    Q_OBJECT

public:
    K3bDiskInfoView( QWidget* parent = 0 );
    ~K3bDiskInfoView();

private:
    void reloadMedium();
    void updateTitle();

    QTextBrowser* m_infoView;
};


#endif
