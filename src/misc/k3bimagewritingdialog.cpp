/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bimagewritingdialog.h"
#include "k3biso9660imagewritingjob.h"
#include "k3bbinimagewritingjob.h"
#include "k3bcuefileparser.h"
#include "k3bclonetocreader.h"
#include "k3baudiocuefilewritingjob.h"
#include <k3bclonejob.h>
#include "k3bmediacache.h"
#include "k3bapplication.h"

#include <config-k3b.h>

#include <k3btempdirselectionwidget.h>
#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3bburnprogressdialog.h>
#include <k3bstdguiitems.h>
#include <k3bmd5job.h>
#include <k3bdatamodewidget.h>
#include <k3bglobals.h>
#include <k3bwritingmodewidget.h>
#include <k3bcore.h>
#include <k3blistview.h>
#include <k3biso9660.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bcdtext.h>

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kstdguiitem.h>
#include <kguiitem.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kurl.h>
#include <kinputdialog.h>
#include <kcombobox.h>

#include <q3header.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qspinbox.h>
#include <qmap.h>
#include <qclipboard.h>
#include <qmenu.h>
#include <QGridLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <K3URLDrag>

class K3b::ImageWritingDialog::Private
{
public:
    Private()
        : md5SumItem(0),
          haveMd5Sum( false ),
          foundImageType( IMAGE_UNKNOWN ),
          imageForced( false ) {
    }

    K3b::ListViewItem* md5SumItem;
    QString lastCheckedFile;

    K3b::Md5Job* md5Job;
    bool haveMd5Sum;

    int foundImageType;

    QMap<int,int> imageTypeSelectionMap;
    QMap<int,int> imageTypeSelectionMapRev;
    QString imageFile;
    QString tocFile;

    QTabWidget* optionTabbed;

    QWidget* advancedTab;
    QWidget* tempPathTab;
    bool advancedTabVisible;
    bool tempPathTabVisible;

    int advancedTabIndex;
    int tempPathTabIndex;

    bool imageForced;
};


K3b::ImageWritingDialog::ImageWritingDialog( QWidget* parent )
    : K3b::InteractionDialog( parent,
                            i18n("Burn Image"),
                            "iso cue toc",
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "image writing" ) // config group
{
    d = new Private();

    setAcceptDrops( true );

    setupGui();

    d->md5Job = new K3b::Md5Job( 0, this );
    connect( d->md5Job, SIGNAL(finished(bool)),
             this, SLOT(slotMd5JobFinished(bool)) );
    connect( d->md5Job, SIGNAL(percent(int)),
             this, SLOT(slotMd5JobPercent(int)) );

    connect( m_writerSelectionWidget, SIGNAL(writerChanged()),
             this, SLOT(slotToggleAll()) );
    connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(K3b::WritingApp)),
             this, SLOT(slotToggleAll()) );
    connect( m_writerSelectionWidget, SIGNAL(writerChanged(K3b::Device::Device*)),
             m_writingModeWidget, SLOT(setDevice(K3b::Device::Device*)) );
    connect( m_comboImageType, SIGNAL(activated(int)),
             this, SLOT(slotToggleAll()) );
    connect( m_writingModeWidget, SIGNAL(writingModeChanged(K3b::WritingMode)),
             this, SLOT(slotToggleAll()) );
    connect( m_editImagePath, SIGNAL(textChanged(const QString&)),
             this, SLOT(slotUpdateImage(const QString&)) );
    connect( m_checkDummy, SIGNAL(toggled(bool)),
             this, SLOT(slotToggleAll()) );
    connect( m_checkCacheImage, SIGNAL(toggled(bool)),
             this, SLOT(slotToggleAll()) );
}


K3b::ImageWritingDialog::~ImageWritingDialog()
{
    d->md5Job->cancel();

    KConfigGroup c( KGlobal::config(), configGroup() );
    QStringList recentImages;
    // do not store more than 10 recent images
    for ( int i = 0; i < m_comboRecentImages->count() && recentImages.count() < 10; ++i ) {
        QString image = m_comboRecentImages->itemText( i );
        if ( !recentImages.contains( image ) )
            recentImages += image;
    }
    c.writePathEntry( "recent images", recentImages );

    delete d;
}


void K3b::ImageWritingDialog::init()
{
    KConfigGroup c( KGlobal::config(), configGroup() );

    if( !d->imageForced ) {
        // when opening the dialog first the default settings are loaded and afterwards we set the
        // last written image because that's what most users want
        QString image = c.readPathEntry( "last written image", QString() );
        if( QFile::exists( image ) )
            m_editImagePath->setUrl( image );
    }

    m_comboRecentImages->clear();
    m_comboRecentImages->addItems( c.readPathEntry( "recent images", QStringList() ) );
}


void K3b::ImageWritingDialog::setupGui()
{
    QWidget* frame = K3b::InteractionDialog::mainWidget();

    // image
    // -----------------------------------------------------------------------
    QGroupBox* groupImageUrl = new QGroupBox( i18n("Image to Burn"), frame );
    m_comboRecentImages = new KComboBox( true, this );
    m_editImagePath = new KUrlRequester( m_comboRecentImages, groupImageUrl );
    m_editImagePath->setMode( KFile::File|KFile::ExistingOnly );
    m_editImagePath->setWindowTitle( i18n("Choose Image File") );
    m_editImagePath->setFilter( i18n("*.iso *.toc *.ISO *.TOC *.cue *.CUE|Image Files")
                                + "\n"
                                + i18n("*.iso *.ISO|ISO9660 Image Files")
                                + "\n"
                                + i18n("*.cue *.CUE|Cue Files")
                                + "\n"
                                + i18n("*.toc *.TOC|Cdrdao TOC Files and Cdrecord Clone Images")
                                + "\n"
                                + i18n("*|All Files") );
    QHBoxLayout* groupImageUrlLayout = new QHBoxLayout( groupImageUrl );
    groupImageUrlLayout->setMargin( 0 );
    groupImageUrlLayout->addWidget( m_editImagePath );

    QGroupBox* groupImageType = new QGroupBox( i18n("Image Type"), frame );
    QHBoxLayout* groupImageTypeLayout = new QHBoxLayout( groupImageType );
    groupImageTypeLayout->setMargin( 0 );
    m_comboImageType = new QComboBox( groupImageType );
    groupImageTypeLayout->addWidget( m_comboImageType );
    groupImageTypeLayout->addStretch( 1 );
    m_comboImageType->addItem( i18n("Auto Detection") );
    m_comboImageType->addItem( i18n("ISO9660 Image") );
    m_comboImageType->addItem( i18n("Cue/Bin Image") );
    m_comboImageType->addItem( i18n("Audio Cue File") );
    m_comboImageType->addItem( i18n("Cdrdao TOC File") );
    m_comboImageType->addItem( i18n("Cdrecord Clone Image") );
    d->imageTypeSelectionMap[1] = IMAGE_ISO;
    d->imageTypeSelectionMap[2] = IMAGE_CUE_BIN;
    d->imageTypeSelectionMap[3] = IMAGE_AUDIO_CUE;
    d->imageTypeSelectionMap[4] = IMAGE_CDRDAO_TOC;
    d->imageTypeSelectionMap[5] = IMAGE_CDRECORD_CLONE;
    d->imageTypeSelectionMapRev[IMAGE_ISO] = 1;
    d->imageTypeSelectionMapRev[IMAGE_CUE_BIN] = 2;
    d->imageTypeSelectionMapRev[IMAGE_AUDIO_CUE] = 3;
    d->imageTypeSelectionMapRev[IMAGE_CDRDAO_TOC] = 4;
    d->imageTypeSelectionMapRev[IMAGE_CDRECORD_CLONE] = 5;


    // image info
    // -----------------------------------------------------------------------
    m_infoView = new K3b::ListView( frame );
    m_infoView->addColumn( "key" );
    m_infoView->addColumn( "value" );
    m_infoView->header()->hide();
    m_infoView->setNoItemText( i18n("No image file selected") );
    m_infoView->setSorting( -1 );
    m_infoView->setAlternateBackground( QColor() );
    m_infoView->setFullWidth(true);
    m_infoView->setSelectionMode( Q3ListView::NoSelection );

    connect( m_infoView, SIGNAL(contextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)),
             this, SLOT(slotContextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)) );


    m_writerSelectionWidget = new K3b::WriterSelectionWidget( frame );
    m_writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE );
    m_writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY );

    // options
    // -----------------------------------------------------------------------
    d->optionTabbed = new QTabWidget( frame );

    QWidget* optionTab = new QWidget( d->optionTabbed );
    QGridLayout* optionTabLayout = new QGridLayout( optionTab );
    optionTabLayout->setAlignment( Qt::AlignTop );

    QGroupBox* writingModeGroup = new QGroupBox( i18n("Writing Mode"), optionTab );
    m_writingModeWidget = new K3b::WritingModeWidget( writingModeGroup );
    QHBoxLayout* writingModeGroupLayout = new QHBoxLayout( writingModeGroup );
    writingModeGroupLayout->addWidget( m_writingModeWidget );

    // copies --------
    QGroupBox* groupCopies = new QGroupBox( i18n("Copies"), optionTab );
    QLabel* pixLabel = new QLabel( groupCopies );
    pixLabel->setPixmap( SmallIcon( "tools-media-optical-copy", KIconLoader::SizeMedium ) );
    pixLabel->setScaledContents( false );
    m_spinCopies = new QSpinBox( groupCopies );
    m_spinCopies->setMinimum( 1 );
    m_spinCopies->setMaximum( 999 );
    QHBoxLayout* groupCopiesLayout = new QHBoxLayout( groupCopies );
    groupCopiesLayout->addWidget( pixLabel );
    groupCopiesLayout->addWidget( m_spinCopies );
    // -------- copies

    QGroupBox* optionGroup = new QGroupBox( i18n("Settings"), optionTab );
    m_checkDummy = K3b::StdGuiItems::simulateCheckbox( optionGroup );
    m_checkCacheImage = K3b::StdGuiItems::createCacheImageCheckbox( optionGroup );
    m_checkVerify = K3b::StdGuiItems::verifyCheckBox( optionGroup );
    QVBoxLayout* optionGroupLayout = new QVBoxLayout( optionGroup );
    optionGroupLayout->addWidget( m_checkDummy );
    optionGroupLayout->addWidget( m_checkCacheImage );
    optionGroupLayout->addWidget( m_checkVerify );
    optionGroupLayout->addStretch( 1 );

    optionTabLayout->addWidget( writingModeGroup, 0, 0 );
    optionTabLayout->addWidget( groupCopies, 1, 0 );
    optionTabLayout->addWidget( optionGroup, 0, 1, 2, 1 );
    optionTabLayout->setRowStretch( 1, 1 );
    optionTabLayout->setColumnStretch( 1, 1 );

    d->optionTabbed->addTab( optionTab, i18n("Settings") );


    // image tab ------------------------------------
    d->tempPathTab = new QWidget( d->optionTabbed );
    QGridLayout* imageTabGrid = new QGridLayout( d->tempPathTab );

    m_tempDirSelectionWidget = new K3b::TempDirSelectionWidget( d->tempPathTab );

    imageTabGrid->addWidget( m_tempDirSelectionWidget, 0, 0 );

    d->tempPathTabIndex = d->optionTabbed->addTab( d->tempPathTab, i18n("&Image") );
    d->tempPathTabVisible = true;
    // -------------------------------------------------------------


    // advanced ---------------------------------
    d->advancedTab = new QWidget( d->optionTabbed );
    QGridLayout* advancedTabLayout = new QGridLayout( d->advancedTab );
    advancedTabLayout->setAlignment( Qt::AlignTop );

    m_dataModeWidget = new K3b::DataModeWidget( d->advancedTab );
    m_checkNoFix = K3b::StdGuiItems::startMultisessionCheckBox( d->advancedTab );

    advancedTabLayout->addWidget( new QLabel( i18n("Data mode:"), d->advancedTab ), 0, 0 );
    advancedTabLayout->addWidget( m_dataModeWidget, 0, 1 );
    advancedTabLayout->addWidget( m_checkNoFix, 1, 0, 1, 3 );
    advancedTabLayout->setRowStretch( 2, 1 );
    advancedTabLayout->setColumnStretch( 2, 1 );

    d->advancedTabIndex = d->optionTabbed->addTab( d->advancedTab, i18n("Advanced") );
    d->advancedTabVisible = true;
    // -----------------------------------------------------------------------




    QGridLayout* grid = new QGridLayout( frame );
    grid->setMargin( 0 );

    grid->addWidget( groupImageUrl, 0, 0 );
    grid->addWidget( groupImageType, 0, 1 );
    grid->setColumnStretch( 0, 1 );
    grid->addWidget( m_infoView, 1, 0, 1, 2 );
    grid->addWidget( m_writerSelectionWidget, 2, 0, 1, 2 );
    grid->addWidget( d->optionTabbed, 3, 0, 1, 2 );

    grid->setRowStretch( 1, 1 );
}


void K3b::ImageWritingDialog::slotStartClicked()
{
    d->md5Job->cancel();

    // save the path
    KConfigGroup grp( KGlobal::config(), configGroup() );
    grp.writePathEntry( "last written image", imagePath() );

    if( d->imageFile.isEmpty() )
        d->imageFile = imagePath();
    if( d->tocFile.isEmpty() )
        d->tocFile = imagePath();

    // create a progresswidget
    K3b::BurnProgressDialog dlg( kapp->activeWindow() );

    // create the job
    K3b::BurnJob* job = 0;
    switch( currentImageType() ) {
    case IMAGE_CDRECORD_CLONE:
    {
        K3b::CloneJob* _job = new K3b::CloneJob( &dlg, this );
        _job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
        _job->setImagePath( d->imageFile );
        _job->setSimulate( m_checkDummy->isChecked() );
        _job->setWriteSpeed( m_writerSelectionWidget->writerSpeed() );
        _job->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );
        _job->setOnlyBurnExistingImage( true );

        job = _job;
    }
    break;

    case IMAGE_AUDIO_CUE:
    {
        K3b::AudioCueFileWritingJob* job_ = new K3b::AudioCueFileWritingJob( &dlg, this );

        job_->setBurnDevice( m_writerSelectionWidget->writerDevice() );
        job_->setSpeed( m_writerSelectionWidget->writerSpeed() );
        job_->setSimulate( m_checkDummy->isChecked() );
        job_->setWritingMode( m_writingModeWidget->writingMode() );
        job_->setCueFile( d->tocFile );
        job_->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );
        job_->setOnTheFly( !m_checkCacheImage->isChecked() );
        job_->setTempDir( m_tempDirSelectionWidget->tempPath() );

        job = job_;
    }
    break;

    case IMAGE_CUE_BIN:
        // for now the K3b::BinImageWritingJob decides if it's a toc or a cue file
    case IMAGE_CDRDAO_TOC:
    {
        K3b::BinImageWritingJob* job_ = new K3b::BinImageWritingJob( &dlg, this );

        job_->setWriter( m_writerSelectionWidget->writerDevice() );
        job_->setSpeed( m_writerSelectionWidget->writerSpeed() );
        job_->setTocFile( d->tocFile );
        job_->setSimulate(m_checkDummy->isChecked());
        job_->setMulti( false /*m_checkNoFix->isChecked()*/ );
        job_->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );

        job = job_;
    }
    break;

    case IMAGE_ISO:
    {
        K3b::Iso9660 isoFs( d->imageFile );
        if( isoFs.open() ) {
            if( K3b::filesize( KUrl(d->imageFile) ) < (KIO::filesize_t)(isoFs.primaryDescriptor().volumeSpaceSize*2048) ) {
                if( KMessageBox::questionYesNo( this,
                                                i18n("<p>This image has an invalid file size. "
                                                     "If it has been downloaded make sure the download is complete."
                                                     "<p>Only continue if you know what you are doing."),
                                                i18n("Warning"),
                                                KGuiItem( i18n("Continue") ),
                                                KGuiItem( i18n("Cancel") ) ) == KMessageBox::No )
                    return;
            }
        }

        K3b::Iso9660ImageWritingJob* job_ = new K3b::Iso9660ImageWritingJob( &dlg );

        job_->setBurnDevice( m_writerSelectionWidget->writerDevice() );
        job_->setSpeed( m_writerSelectionWidget->writerSpeed() );
        job_->setSimulate( m_checkDummy->isChecked() );
        job_->setWritingMode( m_writingModeWidget->writingMode() );
        job_->setVerifyData( m_checkVerify->isChecked() );
        job_->setNoFix( m_checkNoFix->isChecked() );
        job_->setDataMode( m_dataModeWidget->dataMode() );
        job_->setImagePath( d->imageFile );
        job_->setCopies( m_checkDummy->isChecked() ? 1 : m_spinCopies->value() );

        job = job_;
    }
    break;

    default:
        kDebug() << "(K3b::ImageWritingDialog) this should really not happen!";
        break;
    }

    if( job ) {
        job->setWritingApp( m_writerSelectionWidget->writingApp() );

        hide();

        dlg.startJob(job);

        delete job;

        if( KConfigGroup( KGlobal::config(), "General Options" ).readEntry( "keep action dialogs open", false ) )
            show();
        else
            close();
    }
}


void K3b::ImageWritingDialog::slotUpdateImage( const QString& )
{
    QString path = imagePath();

    // check the image types

    d->haveMd5Sum = false;
    d->md5Job->cancel();
    m_infoView->clear();
    m_infoView->header()->resizeSection( 0, 20 );
    d->md5SumItem = 0;
    d->foundImageType = IMAGE_UNKNOWN;
    d->tocFile.truncate(0);
    d->imageFile.truncate(0);

    QFileInfo info( path );
    if( info.isFile() ) {

        // ------------------------------------------------
        // Test for iso9660 image
        // ------------------------------------------------
        K3b::Iso9660 isoF( path );
        if( isoF.open() ) {
#ifdef K3B_DEBUG
            isoF.debug();
#endif

            createIso9660InfoItems( &isoF );
            isoF.close();
            calculateMd5Sum( path );

            d->foundImageType = IMAGE_ISO;
            d->imageFile = path;
        }

        if( d->foundImageType == IMAGE_UNKNOWN ) {

            // check for cdrecord clone image
            // try both path and path.toc as tocfiles
            K3b::CloneTocReader cr;

            if( path.right(4) == ".toc" ) {
                cr.openFile( path );
                if( cr.isValid() ) {
                    d->tocFile = path;
                    d->imageFile = cr.imageFilename();
                }
            }
            if( d->imageFile.isEmpty() ) {
                cr.openFile( path + ".toc" );
                if( cr.isValid() ) {
                    d->tocFile = cr.filename();
                    d->imageFile = cr.imageFilename();
                }
            }

            if( !d->imageFile.isEmpty() ) {
                // we have a cdrecord clone image
                createCdrecordCloneItems( d->tocFile, d->imageFile );
                calculateMd5Sum( d->imageFile );

                d->foundImageType = IMAGE_CDRECORD_CLONE;
            }
        }

        if( d->foundImageType == IMAGE_UNKNOWN ) {

            // check for cue/bin stuff
            // once again we try both path and path.cue
            K3b::CueFileParser cp;

            if( path.right(4).toLower() == ".cue" )
                cp.openFile( path );
            else if( path.right(4).toLower() == ".bin" )
                cp.openFile( path.left( path.length()-3) + "cue" );

            if( cp.isValid() ) {
                d->tocFile = cp.filename();
                d->imageFile = cp.imageFilename();
            }

            if( d->imageFile.isEmpty() ) {
                cp.openFile( path + ".cue" );
                if( cp.isValid() ) {
                    d->tocFile = cp.filename();
                    d->imageFile = cp.imageFilename();
                }
            }

            if( !d->imageFile.isEmpty() ) {
                // we have a cue file
                if( cp.toc().contentType() == K3b::Device::AUDIO ) {
                    d->foundImageType = IMAGE_AUDIO_CUE;
                    createAudioCueItems( cp );
                }
                else {
                    d->foundImageType = IMAGE_CUE_BIN;  // we cannot be sure if writing will work... :(
                    createCueBinItems( d->tocFile, d->imageFile );
                    calculateMd5Sum( d->imageFile );
                }
            }
        }

        if( d->foundImageType == IMAGE_UNKNOWN ) {
            // TODO: check for cdrdao tocfile
        }



        if( d->foundImageType == IMAGE_UNKNOWN ) {
            K3b::ListViewItem* item = new K3b::ListViewItem( m_infoView, m_infoView->lastItem(),
                                                         i18n("Seems not to be a usable image") );
            item->setForegroundColor( 0, Qt::red );
            item->setPixmap( 0, SmallIcon( "dialog-error") );
        }
        else {
            // remember as recent image
            int i = 0;
            while ( i < m_comboRecentImages->count() && m_comboRecentImages->itemText(i) != path )
                ++i;
            if ( i == m_comboRecentImages->count() )
                m_comboRecentImages->insertItem( 0, path );
        }
    }
    else {
        K3b::ListViewItem* item = new K3b::ListViewItem( m_infoView, m_infoView->lastItem(),
                                                     i18n("File not found") );
        item->setForegroundColor( 0, Qt::red );
        item->setPixmap( 0, SmallIcon( "dialog-error") );
    }

    slotToggleAll();
}


void K3b::ImageWritingDialog::createIso9660InfoItems( K3b::Iso9660* isoF )
{
    QColor infoTextColor = palette().color( QPalette::Disabled, QPalette::Text );

    K3b::ListViewItem* isoRootItem = new K3b::ListViewItem( m_infoView, m_infoView->lastItem(),
                                                        i18n("Detected:"),
                                                        i18n("Iso9660 image") );
    isoRootItem->setForegroundColor( 0, infoTextColor );
    isoRootItem->setPixmap( 0, SmallIcon( "application-x-cd-image") );

    KIO::filesize_t size = K3b::filesize( KUrl(isoF->fileName()) );
    K3b::ListViewItem* item = new K3b::ListViewItem( isoRootItem, m_infoView->lastItem(),
                                                 i18n("Filesize:"),
                                                 KIO::convertSize( size ) );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("System Id:"),
                                isoF->primaryDescriptor().systemId.isEmpty()
                                ? QString("-")
                                : isoF->primaryDescriptor().systemId );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("Volume Id:"),
                                isoF->primaryDescriptor().volumeId.isEmpty()
                                ? QString("-")
                                : isoF->primaryDescriptor().volumeId );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("Volume Set Id:"),
                                isoF->primaryDescriptor().volumeSetId.isEmpty()
                                ? QString("-")
                                : isoF->primaryDescriptor().volumeSetId );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("Publisher Id:"),
                                isoF->primaryDescriptor().publisherId.isEmpty()
                                ? QString("-")
                                : isoF->primaryDescriptor().publisherId );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("Preparer Id:"),
                                isoF->primaryDescriptor().preparerId.isEmpty()
                                ? QString("-") : isoF->primaryDescriptor().preparerId );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("Application Id:"),
                                isoF->primaryDescriptor().applicationId.isEmpty()
                                ? QString("-")
                                : isoF->primaryDescriptor().applicationId );
    item->setForegroundColor( 0, infoTextColor );

    isoRootItem->setOpen( true );
}


void K3b::ImageWritingDialog::createCdrecordCloneItems( const QString& tocFile, const QString& imageFile )
{
    QColor infoTextColor = palette().color( QPalette::Disabled, QPalette::Text );

    K3b::ListViewItem* isoRootItem = new K3b::ListViewItem( m_infoView, m_infoView->lastItem(),
                                                        i18n("Detected:"),
                                                        i18n("Cdrecord clone image") );
    isoRootItem->setForegroundColor( 0, infoTextColor );
    isoRootItem->setPixmap( 0, SmallIcon( "application-x-cd-image") );

    K3b::ListViewItem* item = new K3b::ListViewItem( isoRootItem, m_infoView->lastItem(),
                                                 i18n("Filesize:"), KIO::convertSize( K3b::filesize(KUrl(imageFile)) ) );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("Image file:"),
                                imageFile );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("TOC file:"),
                                tocFile );
    item->setForegroundColor( 0, infoTextColor );

    isoRootItem->setOpen( true );
}


void K3b::ImageWritingDialog::createCueBinItems( const QString& cueFile, const QString& imageFile )
{
    QColor infoTextColor = palette().color( QPalette::Disabled, QPalette::Text );

    K3b::ListViewItem* isoRootItem = new K3b::ListViewItem( m_infoView, m_infoView->lastItem(),
                                                        i18n("Detected:"),
                                                        i18n("Cue/bin image") );
    isoRootItem->setForegroundColor( 0, infoTextColor );
    isoRootItem->setPixmap( 0, SmallIcon( "application-x-cd-image") );

    K3b::ListViewItem* item = new K3b::ListViewItem( isoRootItem, m_infoView->lastItem(),
                                                 i18n("Filesize:"), KIO::convertSize( K3b::filesize(KUrl(imageFile)) ) );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("Image file:"),
                                imageFile );
    item->setForegroundColor( 0, infoTextColor );

    item = new K3b::ListViewItem( isoRootItem,
                                m_infoView->lastItem(),
                                i18n("Cue file:"),
                                cueFile );
    item->setForegroundColor( 0, infoTextColor );

    isoRootItem->setOpen( true );
}


void K3b::ImageWritingDialog::createAudioCueItems( const K3b::CueFileParser& cp )
{
    QColor infoTextColor = palette().color( QPalette::Disabled, QPalette::Text );

    K3b::ListViewItem* rootItem = new K3b::ListViewItem( m_infoView, m_infoView->lastItem(),
                                                     i18n("Detected:"),
                                                     i18n("Audio Cue Image") );
    rootItem->setForegroundColor( 0, infoTextColor );
    rootItem->setPixmap( 0, SmallIcon( "audio-x-generic") );

    K3b::ListViewItem* trackParent = new K3b::ListViewItem( rootItem,
                                                        i18np("%1 track", "%1 tracks", cp.toc().count() ),
                                                        cp.toc().length().toString() );
    if( !cp.cdText().isEmpty() )
        trackParent->setText( 1,
                              QString("%1 (%2 - %3)")
                              .arg(trackParent->text(1))
                              .arg(cp.cdText().performer())
                              .arg(cp.cdText().title()) );

    int i = 1;
    foreach( const K3b::Device::Track& track, cp.toc() ) {

        K3b::ListViewItem* trackItem =
            new K3b::ListViewItem( trackParent, m_infoView->lastItem(),
                                 i18n("Track") + " " + QString::number(i).rightJustified( 2, '0' ),
                                 "    " + ( i < cp.toc().count()
                                            ? track.length().toString()
                                            : QString("??:??:??") ) );

        if( !cp.cdText().isEmpty() && (cp.cdText().count() > 0) &&!cp.cdText()[i-1].isEmpty() )
            trackItem->setText( 1,
                                QString("%1 (%2 - %3)")
                                .arg(trackItem->text(1))
                                .arg(cp.cdText()[i-1].performer())
                                .arg(cp.cdText()[i-1].title()) );

        ++i;
    }

    rootItem->setOpen( true );
    trackParent->setOpen( true );
}


void K3b::ImageWritingDialog::toggleAll()
{
    // enable the Write-Button if we found a valid image or the user forced an image type
    setButtonEnabled( START_BUTTON, m_writerSelectionWidget->writerDevice()
                      && currentImageType() != IMAGE_UNKNOWN
                      && QFile::exists( imagePath() ) );

    K3b::Medium medium = k3bappcore->mediaCache()->medium( m_writerSelectionWidget->writerDevice() );

    // set usable writing apps
    // for now we only restrict ourselves in case of CD images
    switch( currentImageType() ) {
    case IMAGE_CDRDAO_TOC:
        m_writerSelectionWidget->setSupportedWritingApps( K3b::WritingAppCdrdao );
        break;
    case IMAGE_CDRECORD_CLONE:
        m_writerSelectionWidget->setSupportedWritingApps( K3b::WritingAppCdrecord );
        break;
    default: {
        K3b::WritingApps apps = K3b::WritingAppCdrecord;
        if ( currentImageType() == IMAGE_ISO ) {
            // DVD/BD is always ISO here
            apps |= K3b::WritingAppGrowisofs;
        }
        if ( K3b::Device::isCdMedia( medium.diskInfo().mediaType() ) )
            apps |= K3b::WritingAppCdrdao;

        m_writerSelectionWidget->setSupportedWritingApps( apps );
        break;
    }
    }

    // set a wanted media type (DVD/BD -> only ISO)
    if ( currentImageType() == IMAGE_ISO ) {
        m_writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE );
    }
    else {
        m_writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE_CD );
    }

    // cdrecord clone and cue both need DAO
    if( m_writerSelectionWidget->writingApp() != K3b::WritingAppCdrdao
        && ( currentImageType() == IMAGE_ISO ||
             currentImageType() == IMAGE_AUDIO_CUE ) )
        m_writingModeWidget->determineSupportedModesFromMedium( medium );
    else
        m_writingModeWidget->setSupportedModes( K3b::WritingModeSao );


    // some stuff is only available for iso images
    if( currentImageType() == IMAGE_ISO ) {
        m_checkVerify->show();
        if( !d->advancedTabVisible ) {
            d->advancedTabIndex = d->optionTabbed->addTab( d->advancedTab, i18n("Advanced") );
        }
        d->advancedTabVisible = true;
        if( m_checkDummy->isChecked() ) {
            m_checkVerify->setEnabled( false );
            m_checkVerify->setChecked( false );
        }
        else
            m_checkVerify->setEnabled( true );
    }
    else {
        if( d->advancedTabVisible ) {
            d->optionTabbed->removeTab( d->advancedTabIndex );
        }
        d->advancedTabVisible = false;
        m_checkVerify->hide();
    }


    // and some other stuff only makes sense for audio cues
    if( currentImageType() == IMAGE_AUDIO_CUE ) {
        if( !d->tempPathTabVisible )
            d->tempPathTabIndex = d->optionTabbed->addTab( d->tempPathTab, i18n("&Image") );
        d->tempPathTabVisible = true;
        m_tempDirSelectionWidget->setDisabled( !m_checkCacheImage->isChecked() );
    }
    else {
        if( d->tempPathTabVisible ) {
            d->optionTabbed->removeTab( d->tempPathTabIndex );
        }
        d->tempPathTabVisible = false;
    }
    m_checkCacheImage->setVisible( currentImageType() == IMAGE_AUDIO_CUE );

    m_spinCopies->setEnabled( !m_checkDummy->isChecked() );


    K3b::ListViewItem* item = dynamic_cast<K3b::ListViewItem*>(m_infoView->firstChild());
    if( item )
        item->setForegroundColor( 1,
                                  currentImageType() != d->foundImageType
                                  ? Qt::red
                                  : m_infoView->palette().color( QPalette::Text ) );
}


void K3b::ImageWritingDialog::setImage( const KUrl& url )
{
    d->imageForced = true;
    m_editImagePath->setUrl( url );
}


void K3b::ImageWritingDialog::calculateMd5Sum( const QString& file )
{
    d->haveMd5Sum = false;

    if( !d->md5SumItem )
        d->md5SumItem = new K3b::ListViewItem( m_infoView, m_infoView->firstChild() );

    d->md5SumItem->setText( 0, i18n("Md5 Sum:") );
    d->md5SumItem->setForegroundColor( 0, palette().color( QPalette::Disabled, QPalette::Text ) );
    d->md5SumItem->setProgress( 1, 0 );
    d->md5SumItem->setPixmap( 0, SmallIcon("system-run") );

    if( file != d->lastCheckedFile ) {
        d->lastCheckedFile = file;
        d->md5Job->setFile( file );
        d->md5Job->start();
    }
    else
        slotMd5JobFinished( true );
}


void K3b::ImageWritingDialog::slotMd5JobPercent( int p )
{
    d->md5SumItem->setProgress( 1, p );
}


void K3b::ImageWritingDialog::slotMd5JobFinished( bool success )
{
    if( success ) {
        d->md5SumItem->setText( 1, d->md5Job->hexDigest() );
        d->md5SumItem->setPixmap( 0, SmallIcon("dialog-information") );
        d->haveMd5Sum = true;
    }
    else {
        d->md5SumItem->setForegroundColor( 1, Qt::red );
        if( d->md5Job->hasBeenCanceled() )
            d->md5SumItem->setText( 1, i18n("Calculation canceled") );
        else
            d->md5SumItem->setText( 1, i18n("Calculation failed") );
        d->md5SumItem->setPixmap( 0, SmallIcon("dialog-error") );
        d->lastCheckedFile.truncate(0);
    }

    d->md5SumItem->setDisplayProgressBar( 1, false );
}


void K3b::ImageWritingDialog::slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& pos )
{
    if( !d->haveMd5Sum )
        return;

    QMenu popup;
    QAction *copyItem = popup.addAction( i18n("Copy checksum to clipboard") );
    QAction *compareItem = popup.addAction( i18n("Compare checksum...") );

    QAction *act = popup.exec( pos );

    if( act == compareItem ) {
        bool ok;
        QString md5sumToCompare = KInputDialog::getText( i18n("MD5 Sum Check"),
                                                         i18n("Please insert the MD5 Sum to compare:"),
                                                         QString(),
                                                         &ok,
                                                         this );
        if( ok ) {
            if( md5sumToCompare.toLower().toUtf8() == d->md5Job->hexDigest().toLower() )
                KMessageBox::information( this, i18n("The MD5 Sum of %1 equals that specified.",imagePath()),
                                          i18n("MD5 Sums Equal") );
            else
                KMessageBox::sorry( this, i18n("The MD5 Sum of %1 differs from that specified.",imagePath()),
                                    i18n("MD5 Sums Differ") );
        }
    }
    else if( act == copyItem ) {
        QApplication::clipboard()->setText( d->md5Job->hexDigest().toLower(), QClipboard::Clipboard );
    }
}


void K3b::ImageWritingDialog::loadSettings( const KConfigGroup& c )
{
    m_writingModeWidget->loadConfig( c );
    m_checkDummy->setChecked( c.readEntry("simulate", false ) );
    m_checkNoFix->setChecked( c.readEntry("multisession", false ) );
    m_checkCacheImage->setChecked( !c.readEntry("on_the_fly", true ) );

    m_dataModeWidget->loadConfig(c);

    m_spinCopies->setValue( c.readEntry( "copies", 1 ) );

    m_checkVerify->setChecked( c.readEntry( "verify_data", false ) );

    m_writerSelectionWidget->loadConfig( c );

    if( !d->imageForced ) {
        QString image = c.readPathEntry( "image path", c.readPathEntry( "last written image", QString() ) );
        if( QFile::exists( image ) )
            m_editImagePath->setUrl( image );
    }

    QString imageType = c.readEntry( "image type", "auto" );
    int x = 0;
    if( imageType == "iso9660" )
        x = d->imageTypeSelectionMapRev[IMAGE_ISO];
    else if( imageType == "cue-bin" )
        x = d->imageTypeSelectionMapRev[IMAGE_CUE_BIN];
    else if( imageType == "audio-cue" )
        x = d->imageTypeSelectionMapRev[IMAGE_AUDIO_CUE];
    else if( imageType == "cdrecord-clone" )
        x = d->imageTypeSelectionMapRev[IMAGE_CDRECORD_CLONE];
    else if( imageType == "cdrdao-toc" )
        x = d->imageTypeSelectionMapRev[IMAGE_CDRDAO_TOC];

    m_comboImageType->setCurrentIndex( x );

    m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() );

    slotToggleAll();
}


void K3b::ImageWritingDialog::saveSettings( KConfigGroup c )
{
    m_writingModeWidget->saveConfig( c ),
        c.writeEntry( "simulate", m_checkDummy->isChecked() );
    c.writeEntry( "multisession", m_checkNoFix->isChecked() );
    c.writeEntry( "on_the_fly", !m_checkCacheImage->isChecked() );
    m_dataModeWidget->saveConfig(c);

    c.writeEntry( "verify_data", m_checkVerify->isChecked() );

    m_writerSelectionWidget->saveConfig( c );

    c.writePathEntry( "image path", imagePath() );

    c.writeEntry( "copies", m_spinCopies->value() );

    QString imageType;
    if( m_comboImageType->currentIndex() == 0 )
        imageType = "auto";
    else {
        switch( d->imageTypeSelectionMap[m_comboImageType->currentIndex()] ) {
        case IMAGE_ISO:
            imageType = "iso9660";
            break;
        case IMAGE_CUE_BIN:
            imageType = "cue-bin";
            break;
        case IMAGE_AUDIO_CUE:
            imageType = "audio-cue";
            break;
        case IMAGE_CDRECORD_CLONE:
            imageType = "cdrecord-clone";
            break;
        case IMAGE_CDRDAO_TOC:
            imageType = "cdrdao-toc";
            break;
        }
    }
    c.writeEntry( "image type", imageType );

    if( m_tempDirSelectionWidget->isEnabled() )
        m_tempDirSelectionWidget->saveConfig();
}


int K3b::ImageWritingDialog::currentImageType()
{
    if( m_comboImageType->currentIndex() == 0 )
        return d->foundImageType;
    else
        return d->imageTypeSelectionMap[m_comboImageType->currentIndex()];
}


QString K3b::ImageWritingDialog::imagePath() const
{
    return K3b::convertToLocalUrl( m_editImagePath->url() ).toLocalFile();
}


void K3b::ImageWritingDialog::dragEnterEvent( QDragEnterEvent* e )
{
    e->setAccepted( K3URLDrag::canDecode(e) );
}


void K3b::ImageWritingDialog::dropEvent( QDropEvent* e )
{
    KUrl::List urls;
    K3URLDrag::decode( e, urls );
    m_editImagePath->setUrl( urls.first().toLocalFile() );
}

#include "k3bimagewritingdialog.moc"
