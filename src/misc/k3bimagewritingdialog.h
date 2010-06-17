/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef _K3B_IMAGEWRITINGDIALOG_H_
#define _K3B_IMAGEWRITINGDIALOG_H_

#include "k3binteractiondialog.h"

class KUrl;
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
        ImageWritingDialog( QWidget* = 0 );
        ~ImageWritingDialog();

        void setImage( const KUrl& url );

    protected Q_SLOTS:
        void slotStartClicked();

        void slotMd5JobPercent( int );
        void slotMd5JobFinished( bool );
        void slotContextMenuRequested( const QPoint& pos );
        void slotUpdateImage( const QString& );

    protected:
        void loadSettings( const KConfigGroup& );
        void saveSettings( KConfigGroup );

        void calculateMd5Sum( const QString& );
        void dragEnterEvent( QDragEnterEvent* );
        void dropEvent( QDropEvent* );

        void init();

        void toggleAll();

    private:
        void setupGui();

        class Private;
        Private* d;
    };
}

#endif
