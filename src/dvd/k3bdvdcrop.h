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
class QRadioButton;
class QCheckBox;
class QSlider;
class QCanvasView;
class QCanvas;
class QPainter;
class QLabel;
class QCanvasLine;
class KIntSpinBox;
class K3bDvdCodecData;
class KProcess;
class K3bDvdPreview;
class KShellProcess;

/**
  *@author Sebastian Trueg
  */

class K3bDvdCrop : public QGroupBox  {
   Q_OBJECT
public: 
    K3bDvdCrop(K3bDvdCodecData *data, QWidget *parent=0, const char *name=0);
    ~K3bDvdCrop();
    void initPreview( );
public slots:
    void slotUpdateFinalSize();
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

    K3bDvdPreview *m_preview;
    K3bDvdCodecData *m_data;
    KShellProcess *previewProcess;

    QLabel *m_finalSize;
    QLabel *m_finalAspect;
    void setupGui();
    void setSpinBoxMode( int step );
    void setCorrectedSpinValue( KIntSpinBox *box, int step );
    void updateSpinBox( int step );
private slots:
    void slotParseProcess( KProcess* p, char *buffer, int length);
    void slotSpinTop( int );
    void slotSpinBottom( int );
    void slotSpinLeft( int );
    void slotSpinRight( int );
    void slotResizeMode( int );
};

#endif
