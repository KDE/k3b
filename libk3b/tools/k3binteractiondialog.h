/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_INTERACTION_DIALOG_H_
#define _K3B_INTERACTION_DIALOG_H_

#include <kdialog.h>


class QGridLayout;
class QLabel;
class QPushButton;
class K3bTitleLabel;
class KConfig;


/**
 * This is the base dialog for all the dialogs in K3b that start 
 * some job. Use setMainWidget to set the contents or let mainWidget()
 * create an empty plain page.
 * The default implementations of the slots just emit the 
 * corresponding signals.
 */
class K3bInteractionDialog : public KDialog
{
  Q_OBJECT

 public:
  /**
   * The constructor.
   * loadUserDefaults will be called automatically when the dialog is showing.
   *
   * @param title the text to be displayed in the K3b header (not the widget frame)
   * @param subTitle additional text that will be displayed after the title in smaller size
   * @param buttonMask combination of Buttons
   * @param defaultButton may also be null to deactivate the feature
   * @param configgroup The config group used for the loadUserDefaults and saveUserDefaults methods
   */
  K3bInteractionDialog( QWidget* parent = 0, 
			const char* name = 0, 
			const QString& title = QString::null,
			const QString& subTitle = QString::null,
			int buttonMask = START_BUTTON|CANCEL_BUTTON,
			int defaultButton = START_BUTTON,
			const QString& configgroup = QString::null,
			bool modal = true, 
			WFlags fl = 0 );
  virtual ~K3bInteractionDialog();

  void setMainWidget( QWidget* w );
  void setTitle( const QString& title, const QString& subTitle = QString::null );
  void setDefaultButton( int b );

  /**
   * reimplemented to allow initialization after the dialog has been opened.
   */
  void show();

  /**
   * If no mainWidget has been set a plain page will be created.
   */
  QWidget* mainWidget();

  enum Buttons {
    START_BUTTON = 1,
    SAVE_BUTTON = 2,
    CANCEL_BUTTON = 4
  };

  QSize sizeHint() const;

 signals:
  void started();
  void canceled();
  void saved();

 public slots:
  void setStartButtonText( const QString& text, 
			   const QString& tooltip = QString::null, 
			   const QString& whatsthis = QString::null );
  void setCancelButtonText( const QString& text, 
			    const QString& tooltip = QString::null, 
			    const QString& whatsthis = QString::null );
  void setSaveButtonText( const QString& text, 
			  const QString& tooltip = QString::null, 
			  const QString& whatsthis = QString::null );

 protected slots:
  virtual void slotStartClicked();

  /**
   * The default implementation emits the canceled() signal
   * and calls close()
   */
  virtual void slotCancelClicked();
  virtual void slotSaveClicked();

 protected:
  /**
   * Reimplement this to support the save/load user default buttons.
   * @p config is already set to the correct group.
   *
   * The save/load buttons are only activated if the config group is
   * set in the constructor.
   */
  virtual void saveUserDefaults( KConfig* config );

  /**
   * Reimplement this to support the save/load user default buttons.
   * @p config is already set to the correct group.
   *
   * The save/load buttons are only activated if the config group is
   * set in the constructor.
   */
  virtual void loadUserDefaults( KConfig* config );

  /**
   * Reimplement this to support the "k3b defaults" button.
   * set all GUI options to reasonable defaults.
   */
  virtual void loadK3bDefaults();

  /**
   * This is called after the dialog has been shown.
   * Use this for initialization that should happen
   * when the user already sees the dialog.
   */
  virtual void init() {}

  /**
   * reimplemented from QDialog
   */
  virtual void keyPressEvent( QKeyEvent* );

  K3bTitleLabel* m_labelTitle;
  QPushButton* m_buttonStart;
  QPushButton* m_buttonSave;
  QPushButton* m_buttonCancel;
  QWidget* m_mainWidget;

 private slots:
  void slotLoadK3bDefaults();
  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotLoadLastSettings();
  void slotSaveLastSettings();

 private:
  void initConnections();
  void initToolTipsAndWhatsThis();

  QPushButton* m_buttonK3bDefaults;
  QPushButton* m_buttonUserDefaults;
  QPushButton* m_buttonSaveUserDefaults;

  QGridLayout* mainGrid;
  int m_defaultButton;
  QString m_configGroup;
};

#endif
