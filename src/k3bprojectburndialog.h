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


class K3bDoc;
class K3bBurnJob;
class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class QGroupBox;
class QCheckBox;
class QTabWidget;
class QVBoxLayout;


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
	
 protected slots:
   /** burn */
   virtual void slotOk();
   /** save */
   virtual void slotUser1();
   /** cancel */
   virtual void slotUser2();
   virtual void slotCancel();

   virtual void loadDefaults() = 0;
   virtual void loadUserDefaults() = 0;
   virtual void saveUserDefaults() = 0;

 signals:
   void writerChanged();

 protected:
   virtual void saveSettings() {};
   virtual void readSettings() {};

   /**
    * use this to set additionell stuff in the job
    */
   virtual void prepareJob( K3bBurnJob* ) {};

   /**
    * The widget to add new stuff. Use instead of mainWidget()
    */
   QWidget* k3bMainWidget() { return m_k3bMainWidget; }

   void prepareGui();
   void addPage( QWidget*, const QString& title );

   K3bWriterSelectionWidget* m_writerSelectionWidget;
   K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
   QGroupBox* m_optionGroup;
   QVBoxLayout* m_optionGroupLayout;
   QCheckBox* m_checkDao;
   QCheckBox* m_checkOnTheFly;
   QCheckBox* m_checkBurnproof;
   QCheckBox* m_checkSimulate;

 private:
   K3bDoc* m_doc;
   K3bBurnJob* m_job;
   QWidget* m_k3bMainWidget;
   QTabWidget* m_tabWidget;

   QPushButton* m_buttonLoadDefaults;
   QPushButton* m_buttonSaveUserDefaults;
   QPushButton* m_buttonLoadUserDefaults;
};

#endif
