/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_IMAGEWRITINGDIALOG_H_
#define _K3B_IMAGEWRITINGDIALOG_H_

#include "k3binteractiondialog.h"

class QUrl;
class QDragEnterEvent;
class QDropEvent;
class QPoint;

namespace K3b {
    class Iso9660;
    class CueFileParser;


    /**
     *@author Sebastian Trueg
     */
    class ImageWritingDialog : public InteractionDialog
    {
        Q_OBJECT

    public:
        explicit ImageWritingDialog( QWidget* = 0 );
        ~ImageWritingDialog() override;

        void setImage( const QUrl& url );

    protected Q_SLOTS:
        void slotStartClicked() override;

        void slotMd5JobPercent( int );
        void slotMd5JobFinished( bool );
        void slotContextMenuRequested( const QPoint& pos );
        void slotUpdateImage( const QString& );

    protected:
        void loadSettings( const KConfigGroup& ) override;
        void saveSettings( KConfigGroup ) override;

        void calculateMd5Sum( const QString& );
        void dragEnterEvent( QDragEnterEvent* ) override;
        void dropEvent( QDropEvent* ) override;

        void init() override;

        void toggleAll() override;

    private:
        void setupGui();

        class Private;
        Private* d;
    };
}

#endif
