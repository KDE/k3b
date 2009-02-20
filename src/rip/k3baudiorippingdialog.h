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


#ifndef _K3B_AUDIO_RIPPING_DIALOG_H_
#define _K3B_AUDIO_RIPPING_DIALOG_H_

#include <k3binteractiondialog.h>
#include <k3btoc.h>
#include <k3bmedium.h>

#include <qstringlist.h>
#include <QList>

#include <libkcddb/cdinfo.h>

namespace K3bDevice {
    class Device;
}


class QCheckBox;
class QSpinBox;
class QComboBox;
class K3bCddbPatternWidget;
class K3bAudioConvertingOptionWidget;


/**
 *@author Sebastian Trueg
 */
class K3bAudioRippingDialog : public K3bInteractionDialog
{
    Q_OBJECT

public: 
    K3bAudioRippingDialog( const K3bMedium&,
                           const KCDDB::CDInfo&, 
                           const QList<int>&, 
                           QWidget *parent = 0 );
    ~K3bAudioRippingDialog();

    void setStaticDir( const QString& path );

public Q_SLOTS:  
    void refresh();
    void init();
  
private Q_SLOTS:
    void slotStartClicked();

private:
    K3bMedium m_medium;
    KCDDB::CDInfo m_cddbEntry;
    QList<int> m_trackNumbers;

    QComboBox* m_comboParanoiaMode;
    QSpinBox* m_spinRetries;
    QCheckBox* m_checkIgnoreReadErrors;
    QCheckBox* m_checkUseIndex0;

    K3bCddbPatternWidget* m_patternWidget;
    K3bAudioConvertingOptionWidget* m_optionWidget;

    void setupGui();
    void setupContextHelp();

    void loadK3bDefaults();
    void loadUserDefaults( const KConfigGroup& );
    void saveUserDefaults( KConfigGroup );

    class Private;
    Private* d;
};

#endif
