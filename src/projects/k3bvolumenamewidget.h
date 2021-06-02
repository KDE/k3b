/*

    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3BVOLUMENAMEWIDGET_H
#define K3BVOLUMENAMEWIDGET_H

#include <QWidget>

class QEvent;

namespace K3b {
    
    class DataDoc;
    
    /**
     * Simple widget for editing volume's name of data project.
     * Intended to be used on project view toolbars
     * @author Michal Malek
     */
    class VolumeNameWidget : public QWidget
    {
        Q_OBJECT
        
    public:
        explicit VolumeNameWidget( DataDoc* doc, QWidget* parent = 0 );
        ~VolumeNameWidget() override;
        
    protected:
        void changeEvent( QEvent* event ) override;
        
    private Q_SLOTS:
        void slotDocChanged();
        
    private:
        class Private;
        Private* d;
    };
    
} // namespace K3b

#endif // K3BVOLUMENAMEWIDGET_H
