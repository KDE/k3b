/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/



#ifndef K3BDISKINFOVIEW_H
#define K3BDISKINFOVIEW_H

#include "k3bmediacontentsview.h"

#ifdef HAVE_QTWEBENGINEWIDGETS
class QWebEngineView;
#else
class QTextBrowser;
#endif

namespace K3b {
class DiskInfoView : public MediaContentsView
{
    Q_OBJECT

public:
    explicit DiskInfoView( QWidget* parent = 0 );
    ~DiskInfoView() override;

private:
    void reloadMedium() override;
    void updateTitle();

    QString createMediaInfoItems( const Medium& medium );
    QString createIso9660InfoItems( const Iso9660SimplePrimaryDescriptor& iso );
    QString createTrackItems( const Medium& medium );

#ifdef HAVE_QTWEBENGINEWIDGETS
    QWebEngineView* m_infoView;
#else
    QTextBrowser* m_infoView;
#endif
};
}


#endif
