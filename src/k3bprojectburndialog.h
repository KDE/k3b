/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BPROJECTBURNDIALOG_H
#define K3BPROJECTBURNDIALOG_H

#include <k3binteractiondialog.h>


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
class K3bProjectBurnDialog : public K3bInteractionDialog
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
   virtual void slotStartClicked();
   /** save */
   virtual void slotSaveClicked();
   virtual void slotCancelClicked();

   /**
    * gets called if the user changed the writer
    * default implementation just calls 
    * toggleAllOptions()
    */
   virtual void slotWriterChanged();

   /**
    * gets called if the user changed the writing app
    * default implementation just calls 
    * toggleAllOptions()
    */
   virtual void slotWritingAppChanged( int );

   virtual void toggleAllOptions();

 signals:
   void writerChanged();

 protected:
   virtual void saveSettings() {};
   virtual void readSettings() {};

   /**
    * use this to set additionell stuff in the job
    */
   virtual void prepareJob( K3bBurnJob* ) {};

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
   QCheckBox* m_checkRemoveBufferFiles;
   QCheckBox* m_checkOnlyCreateImage;

 private:
   K3bDoc* m_doc;
   K3bBurnJob* m_job;
   QTabWidget* m_tabWidget;
};

#endif
