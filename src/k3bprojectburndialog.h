/***************************************************************************
                          k3bprojectburndialog.h  -  description
                             -------------------
    begin                : Thu May 17 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BPROJECTBURNDIALOG_H
#define K3BPROJECTBURNDIALOG_H

#include <kdialogbase.h>

class QComboBox;
class QGroupBox;
class QLineEdit;
class QToolButton;
class K3bDoc;
class K3bDevice;

/**
  *@author Sebastian Trueg
  */

class K3bProjectBurnDialog : public KDialogBase
{
   Q_OBJECT

 public:
   K3bProjectBurnDialog(K3bDoc* doc, QWidget *parent=0, const char *name=0, bool modal = true );
   ~K3bProjectBurnDialog();

   enum resultCode { Canceled = 0, Saved = 1, Burn = 2 };

   /** shows the dialog with exec()
       @param burn If true the dialog shows the Burn-button */
   int exec( bool burn );

   K3bDoc* doc() const { return m_doc; }

   QString tempDir() const;
	
 protected slots:
   virtual void slotUser1();
   virtual void slotOk();
   virtual void slotCancel();
   void slotRefreshWriterSpeeds();
   virtual void slotTempDirButtonPressed();
   void slotUpdateFreeTempSpace( const QString& );
   void slotFreeTempSpace(const QString&, unsigned long, unsigned long, unsigned long);
   void setTempDir( const QString& );

 signals:
   void writerChanged();

 protected:
   QGroupBox* writerBox( QWidget* parent = 0 );
   QGroupBox* tempDirBox( QWidget* parent = 0 );

   int writerSpeed() const;
   K3bDevice* writerDevice() const;
	
   virtual void saveSettings() = 0;
   virtual void readSettings();
	
 private:
   QGroupBox* m_groupWriter;
   QComboBox* m_comboSpeed;
   QComboBox* m_comboWriter;

   QGroupBox* m_groupTempDir;
   QLabel* m_labelCdSize;
   QLabel* m_labelFreeSpace;
   QLineEdit* m_editDirectory;
   QToolButton* m_buttonFindIsoImage;

   K3bDoc* m_doc;
};

#endif
