/***************************************************************************
                          k3bbinimagewritingdialog.cpp  -  description
                             -------------------
    begin                : Mon Jan 13 2003
    copyright            : (C) 2003 by Klaus-Dieter Krannich
    email                : kd@math.tu-cottbus.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3b.h"
#include "k3bbinimagewritingdialog.h"
#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3bburnprogressdialog.h"
#include "../k3bstdguiitems.h"
#include "k3bbinimagewritingjob.h"
#include <k3bstdguiitems.h>

#include <kconfig.h>
#include <kstandarddirs.h>
#include <kcutlabel.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtabwidget.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qgroupbox.h>



    // ------------------------------------------------
    // Test for cue/bin
    // ------------------------------------------------

    // TODO: check if cue-file has only one FILE-statement (otherwise say: unusabe cue-file: xxx.cue)

//     if( info.extension( false ) == "bin" || info.extension( false ) == "cue" ) {
//       // search the corresponding file
//       QString basename = path.left( path.length() - 3 );

//       // cdrdao does not use the FILE statement but the basename + the bin extension
//       bool binAvail = QFile::exists( basename + "bin" );
//       bool cueAvail = QFile::exists( basename + "cue" );

//       if( binAvail && cueAvail ) {
// 	// TODO: check if the cue-file has only one FILE-statement as needed for cdrdao
// 	m_bCueBinAvailable = true;
// 	m_cuePath = basename + "cue";
// 	m_binPath = basename + "bin";

// 	imageSize = QFileInfo( m_binPath ).size();
//       }
//     }


K3bBinImageWritingDialog::K3bBinImageWritingDialog( QWidget* parent, const char* name, bool modal )
  : KDialogBase( parent, name, modal, i18n("Write Bin/Cue Image to CD"), User1|User2,
		 User1, false, KGuiItem( i18n("Write"), "write", i18n("Start writing") ), KStdGuiItem::close() )
{
   m_job = 0;
   k3bMain()->config()->setGroup( "General Options" );
   m_tocPath=k3bMain()->config()->readEntry( "Temp Dir", locateLocal( "appdata", "temp/" ) );
   setupGui();
   setButtonBoxOrientation( Qt::Vertical );
}


K3bBinImageWritingDialog::~K3bBinImageWritingDialog()
{
   if (m_job) 
      delete m_job;
}


void K3bBinImageWritingDialog::setupGui()
{
  QFrame* frame = makeMainWidget();

  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );

  // image group box
  // -----------------------------------------------------------------------
  QGroupBox* groupImage = new QGroupBox( i18n("CUE/TOC File"), frame );
  groupImage->setColumnLayout(0, Qt::Vertical );
  groupImage->layout()->setSpacing( 0 );
  groupImage->layout()->setMargin( 0 );
  QGridLayout* groupImageLayout = new QGridLayout( groupImage->layout() );
  groupImageLayout->setAlignment( Qt::AlignTop );
  groupImageLayout->setSpacing( spacingHint() );
  groupImageLayout->setMargin( marginHint() );

  m_editTocPath = new KLineEdit( groupImage );
  
  m_buttonFindTocFile = new QToolButton( groupImage );
  m_buttonFindTocFile->setIconSet( SmallIconSet( "fileopen" ) );

  groupImageLayout->addWidget( m_editTocPath, 0, 0 );
  groupImageLayout->addWidget( m_buttonFindTocFile, 0, 1 );

  // options
  // -----------------------------------------------------------------------
  QTabWidget* tabOptions = new QTabWidget( frame );

  QWidget* optionTab = new QWidget( tabOptions );
  QGridLayout* optionTabLayout = new QGridLayout( optionTab );
  optionTabLayout->setMargin(marginHint());
  optionTabLayout->setSpacing(spacingHint());

  QGroupBox* groupOptions = new QGroupBox( 5, Qt::Vertical, i18n("Options"), optionTab );
  groupOptions->setInsideSpacing( spacingHint() );
  groupOptions->setInsideMargin( marginHint() );

  m_checkSimulate = K3bStdGuiItems::simulateCheckbox( groupOptions );
  m_checkMulti    = new QCheckBox( i18n("Multisession"), groupOptions );

  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), optionTab );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );

  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( locate( "appdata", "pics/k3b_cd_copy.png" ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( groupCopies );
  m_spinCopies->setMinValue( 1 );
  m_spinCopies->setMaxValue( 99 );

  optionTabLayout->addWidget( groupOptions, 0, 0 );
  optionTabLayout->addWidget( groupCopies, 0, 1 );

  tabOptions->addTab( optionTab, i18n("&Options") );

  // advanced tab
  // ------------
  QWidget* advancedTab = new QWidget( tabOptions );
  QGridLayout* advancedTabLayout = new QGridLayout( advancedTab );
  advancedTabLayout->setMargin(marginHint());
  advancedTabLayout->setSpacing(spacingHint());

  m_checkForce = new QCheckBox( i18n("Force Writing"), advancedTab );

  advancedTabLayout->addWidget( m_checkForce, 0, 0 );
  advancedTabLayout->setRowStretch( 1, 1 );

  tabOptions->addTab( advancedTab, i18n("&Advanced") );


  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  grid->addWidget( m_writerSelectionWidget, 0, 0 );
  grid->addWidget( groupImage, 1, 0 );
  grid->addWidget( tabOptions, 2, 0 );

  grid->setRowStretch( 2, 1 );

  connect( m_buttonFindTocFile, SIGNAL(clicked()), this, SLOT(slotFindTocFile()) );

  QToolTip::add( m_checkMulti, i18n("Write Multisession CD, don't close") );
  QToolTip::add( m_checkForce, i18n("Force Writing") );
  QToolTip::add( m_spinCopies, i18n("Number of copies") );

  QWhatsThis::add( m_checkMulti, i18n("<p>If this option is checked, the Image is written as the first "
					 "session of a multisession CD "
				      "<p>It is possible to append another session on such disks.") );
  QWhatsThis::add( m_checkForce, i18n("<p>Forces the execution of an operation that otherwise would not be "
				      "performed. ") );
  QWhatsThis::add( m_spinCopies, i18n("<p>Select how many copies you want K3b to create from the Image.") );

  slotWriterChanged();  
}


void K3bBinImageWritingDialog::slotUser1()
{
  if( m_job == 0 )
    m_job = new K3bBinImageWritingJob();

  m_job->setWriter( m_writerSelectionWidget->writerDevice() );
  m_job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  m_job->setTocFile(m_editTocPath->text());
  m_job->setSimulate(m_checkSimulate->isChecked());
  m_job->setMulti(m_checkMulti->isChecked());
  m_job->setForce(m_checkForce->isChecked());
  m_job->setCopies(m_spinCopies->value());

  if (!m_editTocPath->text().isEmpty()) {

     K3bBurnProgressDialog d( kapp->mainWidget(), "burnProgress", true );
  
     d.setJob( m_job );
     hide();

     m_job->start();
     d.exec();
     slotClose();
  } else
     KMessageBox::error( this, i18n("Please select a TOC File"), i18n("No TOC File"));
}


void K3bBinImageWritingDialog::slotUser2()
{
  slotClose();
}


void K3bBinImageWritingDialog::setTocFile( const KURL& url )
{
  m_tocPath = url.path();
}

void K3bBinImageWritingDialog::slotFindTocFile()
{  
  QString newPath( KFileDialog::getOpenFileName( m_tocPath, 
                                                 QString::null, this, 
                                                 i18n("Choose TOC/CUE file") ) );
  if( !newPath.isEmpty() )
    m_editTocPath->setText( newPath );
}

void K3bBinImageWritingDialog::slotWriterChanged()
{
}

#include "k3bbinimagewritingdialog.moc"
