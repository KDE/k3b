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


#ifndef K3BDATAPROPERTIESDIALOG_H
#define K3BDATAPROPERTIESDIALOG_H

#include <kdialog.h>

//Added by qt3to4:
#include <QFrame>
#include <QLabel>
#include <QList>

namespace K3b {
    class DataItem;
}

class KLineEdit;
class QLabel;
class QCheckBox;


/**
 *@author Sebastian Trueg
 */
namespace K3b {
class DataPropertiesDialog : public KDialog  
{
    Q_OBJECT

public: 
    DataPropertiesDialog( const QList<DataItem*>&, QWidget* parent = 0 );
    ~DataPropertiesDialog();

protected Q_SLOTS:
    void slotOk();

private:
    KLineEdit* m_editName;
    QLabel* m_multiSelectionLabel;
    QLabel* m_labelIcon;
    QLabel* m_labelType;
    QLabel* m_labelLocation;
    QLabel* m_labelSize;
    QLabel* m_labelBlocks;
    QLabel* m_extraInfoLabel;

    QFrame* m_spacerLine;

    QLabel* m_labelLocalNameText;
    QLabel* m_labelLocalLocationText;
    QLabel* m_labelLocalName;
    QLabel* m_labelLocalLocation;

    QCheckBox* m_checkHideOnRockRidge;
    QCheckBox* m_checkHideOnJoliet;
    KLineEdit* m_editSortWeight;

    QList<DataItem*> m_dataItems;

    void loadItemProperties( DataItem* );
    void loadListProperties( const QList<DataItem*>& );
};
}

#endif
