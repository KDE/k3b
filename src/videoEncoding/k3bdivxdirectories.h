/***************************************************************************
                          k3bdvddirectories.h  -  description
                             -------------------
    begin                : Sun Mar 31 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BDVDDIRECTORIES_H
#define K3BDVDDIRECTORIES_H

#include <qgroupbox.h>
#include <qstring.h>

class KLineEdit;
class QToolButton;
class K3bDivxCodecData;
class K3bDivXDataGui;

/**
  *@author Sebastian Trueg
  */

class K3bDivxDirectories : public QGroupBox  {
    Q_OBJECT
public:
    K3bDivxDirectories( K3bDivxCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDivxDirectories();
signals:
    void dataChanged( );
private:
    KLineEdit *m_editVideoPath;
    QToolButton *m_buttonVideoDir;
    KLineEdit *m_editAudioPath;
    QToolButton *m_buttonAudioDir;
    KLineEdit *m_editAviPath;
    QToolButton *m_buttonAviDir;
    K3bDivxCodecData *m_data;

    void setupGui();

private slots:
    void slotAviClicked();
    void slotAudioClicked();
    void slotVideoClicked();
    void slotVideoEdited( const QString& );
    void slotAviEdited( const QString& );
};

#endif
