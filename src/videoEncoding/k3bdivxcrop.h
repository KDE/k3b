/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

/***************************************************************************
                          k3bdvdcrop.h  -  description
                             -------------------
    begin                : Tue Apr 2 2002
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

#ifndef K3BDVDCROP_H
#define K3BDVDCROP_H

#include <qwidget.h>
#include <qgroupbox.h>
#include <kdirsize.h>

class QRadioButton;
class QCheckBox;
class QSlider;
class QCanvasView;
class QCanvas;
class QPainter;
class QLabel;
class QImage;
class QCanvasLine;
class KIntSpinBox;
class K3bDivxCodecData;
class KProcess;
class K3bDivxPreview;
class KShellProcess;

/**
  *@author Sebastian Trueg
  */

class K3bDivxCrop : public QGroupBox  {
   Q_OBJECT
public:
    K3bDivxCrop(K3bDivxCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDivxCrop();
    void initPreview( );
    void resetView();
public slots:
    void slotUpdateFinalSize();
    void slotAutoCropMode( int buttonStatus );
    void slotEncodePreview();
    void slotUpdateView();
signals:
    void cropChanged();
    void previewEncoded();
protected:
//   void drawContents( QPainter* p );
private:
    QRadioButton *m_buttonExactly;
    QRadioButton *m_buttonFast;
    QCheckBox *m_autoCrop;
    KIntSpinBox *m_spinTop;
    KIntSpinBox *m_spinBottom;
    KIntSpinBox *m_spinLeft;
    KIntSpinBox *m_spinRight;
    QSlider *m_sliderPreview;

    K3bDivxPreview *m_preview;
    K3bDivxCodecData *m_data;
    KShellProcess *previewProcess;

    QLabel *m_finalSize;
    QLabel *m_finalAspect;
    QLabel *m_finalQuality;
     // how many (2*kbytes) should move on movie (transcode -L parameter)
    int m_previewOffset;
    KIO::filesize_t m_maxDirSize;
    bool m_firstPictureLine;
    bool m_tcDvdModeChanged;

    void setupGui();
    void initSlider();
    void setSpinBoxMode( int step );
    void setCorrectedSpinValue( KIntSpinBox *box, int step );
    void updateSpinBox( int step );
    void encodePreview();
    void copyIfos();
    int checkMaxValidSpinValue( KIntSpinBox *box , int);
    void enableManuelCrop( bool );
    void autoCrop();
    bool checkLine( int difArray[ 25 ] );
    int checkPixels( QImage *i, int x, int y, int xoffset, int yoffset );
    void resetCrop();

private slots:
    void slotParseProcess( KProcess* p, char *buffer, int length);
    void slotSpinTop( int );
    void slotSpinBottom( int );
    void slotSpinLeft( int );
    void slotSpinRight( int );
    void slotResizeMode( int );
    void slotPreviewChanged( int );
    void slotTcDvdModeGroup( int id );
};

#endif
