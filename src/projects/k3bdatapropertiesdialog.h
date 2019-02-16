/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


#ifndef K3BDATAPROPERTIESDIALOG_H
#define K3BDATAPROPERTIESDIALOG_H

#include <QList>
#include <QDialog>

class KLineEdit;
class KSqueezedTextLabel;
class QFrame;
class QLabel;
class QCheckBox;

/**
 *@author Sebastian Trueg
 */
namespace K3b {
    class DataItem;

    class DataPropertiesDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit DataPropertiesDialog( const QList<DataItem*>&, QWidget* parent = 0 );
        ~DataPropertiesDialog() override;

    protected Q_SLOTS:
        void accept() override;

    private:
        KLineEdit* m_editName;
        QLabel* m_multiSelectionLabel;
        QLabel* m_labelIcon;
        QLabel* m_labelType;
        KSqueezedTextLabel* m_labelLocation;
        QLabel* m_labelSize;
        QLabel* m_labelBlocks;
        QLabel* m_extraInfoLabel;

        QFrame* m_spacerLine;

        QLabel* m_labelLocalNameText;
        QLabel* m_labelLocalLocationText;
        QLabel* m_labelLocalLinkTargetText;
        KSqueezedTextLabel* m_labelLocalName;
        KSqueezedTextLabel* m_labelLocalLocation;
        KSqueezedTextLabel* m_labelLocalLinkTarget;

        QCheckBox* m_checkHideOnRockRidge;
        QCheckBox* m_checkHideOnJoliet;
        KLineEdit* m_editSortWeight;

        QList<DataItem*> m_dataItems;

        void loadItemProperties( DataItem* );
        void loadListProperties( const QList<DataItem*>& );
    };
}

#endif
