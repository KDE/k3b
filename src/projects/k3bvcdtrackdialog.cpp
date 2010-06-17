/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

// K3b Includes
#include "k3bvcdtrackdialog.h"
#include "k3bvcdtrack.h"
#include "k3bmsf.h"
#include "k3bglobals.h"
#include "k3bvcdtrackkeysdelegate.h"
#include "k3bvcdtrackkeysmodel.h"

// Kde Includes
#include <KIconLoader>
#include <kio/global.h>
#include <KLocale>
#include <KMimeType>
#include <KNumInput>
#include <KUrl>
#include <KSqueezedTextLabel>

// Qt Includes
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPixmap>
#include <QRadioButton>
#include <QToolTip>
#include <QTreeView>


K3b::VcdTrackDialog::VcdTrackDialog( K3b::VcdDoc* _doc, QList<K3b::VcdTrack*>& tracks, QList<K3b::VcdTrack*>& selectedTracks, QWidget* parent )
    : KDialog( parent ),
      m_vcdDoc( _doc ),
      m_tracks( tracks ),
      m_selectedTracks( selectedTracks )
{
    setCaption( i18n( "Video Track Properties" ) );
    setButtons( Ok|Apply|Cancel );
    setDefaultButton( Ok );

    prepareGui();

    setupPbcTab();
    setupPbcKeyTab();
    setupVideoTab();
    setupAudioTab();

    if ( !m_selectedTracks.isEmpty() ) {

        K3b::VcdTrack * selectedTrack = m_selectedTracks.first();

        m_displayFileName->setText( selectedTrack->fileName() );
        m_displayLength->setText( selectedTrack->duration() );
        m_displaySize->setText( KIO::convertSize( selectedTrack->size() ) );
        m_muxrate->setText( i18n( "%1 bit/s", selectedTrack->muxrate() ) );

        if ( selectedTrack->isSegment() )
            m_labelMimeType->setPixmap( SmallIcon( "image-x-generic", KIconLoader::SizeMedium ) );
        else
            m_labelMimeType->setPixmap( SmallIcon( "video-x-generic", KIconLoader::SizeMedium ) );

        fillGui();
    }
    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect(this,SIGNAL(applyClicked()),this,SLOT(slotApply()));
}

K3b::VcdTrackDialog::~VcdTrackDialog()
{}

void K3b::VcdTrackDialog::slotOk()
{
    slotApply();
    done( 0 );
}

void K3b::VcdTrackDialog::setPbcTrack( K3b::VcdTrack* selected, K3b::CutComboBox* box, int which )
{
    // TODO: Unset Userdefined on default settings
    kDebug() << QString( "K3b::VcdTrackDialog::setPbcTrack: currentIndex = %1, count = %2" ).arg( box->currentIndex() ).arg( m_tracks.count() );

    int count = m_tracks.count();

    if ( selected->getPbcTrack( which ) == m_tracks.at( box->currentIndex() ) ) {
        if ( selected->getNonPbcTrack( which ) == ( int ) ( box->currentIndex() - count ) ) {
            kDebug() << "K3b::VcdTrackDialog::setPbcTrack: not changed, return";
            return ;
        }
    }

    if ( selected->getPbcTrack( which ) )
        selected->getPbcTrack( which ) ->delFromRevRefList( selected );

    if ( box->currentIndex() > count - 1 ) {
        selected->setPbcTrack( which );
        selected->setPbcNonTrack( which, box->currentIndex() - count );
    } else {
        selected->setPbcTrack( which, m_tracks.at( box->currentIndex() ) );
        m_tracks.at( box->currentIndex() ) ->addToRevRefList( selected );
    }

    selected->setUserDefined( which, true );
}

void K3b::VcdTrackDialog::slotApply()
{
    // track set
    K3b::VcdTrack * selectedTrack = m_selectedTracks.first();

    setPbcTrack( selectedTrack, m_pbc_previous, K3b::VcdTrack::PREVIOUS );
    setPbcTrack( selectedTrack, m_pbc_next, K3b::VcdTrack::NEXT );
    setPbcTrack( selectedTrack, m_pbc_return, K3b::VcdTrack::RETURN );
    setPbcTrack( selectedTrack, m_pbc_default, K3b::VcdTrack::DEFAULT );
    setPbcTrack( selectedTrack, m_comboAfterTimeout, K3b::VcdTrack::AFTERTIMEOUT );

    selectedTrack->setPlayTime( m_spin_times->value() );
    selectedTrack->setWaitTime( m_spin_waittime->value() );
    selectedTrack->setReactivity( m_check_reactivity->isChecked() );
    selectedTrack->setPbcNumKeys( m_check_usekeys->isChecked() );
    selectedTrack->setPbcNumKeysUserdefined( m_check_overwritekeys->isChecked() );

    // global set
    VcdOptions() ->setPbcEnabled( m_check_pbc->isChecked() );

    // define numeric keys
    selectedTrack->delDefinedNumKey();

    if ( m_check_overwritekeys->isChecked() ) {
        
        for( int key = 1; key <= m_keys_model->keyCount(); ++key ) {
            VcdTrackKeysModel::Key2Track::const_iterator it = m_keys_model->selectedKeys().constFind( key );
            if( it != m_keys_model->selectedKeys().constEnd() )
                selectedTrack->setDefinedNumKey( it.key(), it.value() );
            else
                selectedTrack->delDefinedNumKey( it.key() );
        }
    } else {
        selectedTrack->setDefinedNumKey( 1, selectedTrack );
        kDebug() << "Key 1" << " Playing: (default) " << VcdTrackKeysModel::trackName( selectedTrack, selectedTrack ) << "Track: " << selectedTrack;
    }
}

void K3b::VcdTrackDialog::fillGui()
{
    K3b::VcdTrack * selectedTrack = m_selectedTracks.first();

    m_mpegver_video->setText( selectedTrack->mpegTypeS() );
    m_rate_video->setText( selectedTrack->video_bitrate() );
    m_chromaformat_video->setText( selectedTrack->video_chroma() );
    m_format_video->setText( selectedTrack->video_format() );
    m_highresolution_video->setText( selectedTrack->highresolution() );
    m_resolution_video->setText( selectedTrack->resolution() );

    m_mpegver_audio->setText( selectedTrack->mpegTypeS( true ) );
    m_rate_audio->setText( selectedTrack->audio_bitrate() );

    m_sampling_frequency_audio->setText( selectedTrack->audio_sampfreq() );
    m_mode_audio->setText( selectedTrack->audio_mode() );
    m_copyright_audio->setText( selectedTrack->audio_copyright() );

    fillPbcGui();


    m_pbc_previous->setToolTip( i18n( "May also look like | << on the remote control. " ) );
    m_pbc_next->setToolTip( i18n( "May also look like >> | on the remote control." ) );
    m_pbc_return->setToolTip( i18n( "This key may be mapped to the STOP key." ) );
    m_pbc_default->setToolTip( i18n( "This key is usually mapped to the > or PLAY key." ) );
    m_comboAfterTimeout->setToolTip( i18n( "Target to be jumped to on time-out of <wait>." ) );
    m_check_reactivity->setToolTip( i18n( "Delay reactivity of keys." ) );
    m_check_pbc->setToolTip( i18n( "Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats." ) );
    m_check_usekeys->setToolTip( i18n( "Activate the use of numeric keys." ) );
    m_check_overwritekeys->setToolTip( i18n( "Overwrite default numeric keys." ) );
    m_keys_view->setToolTip( i18n( "Numeric keys." ) );
    m_spin_times->setToolTip( i18n( "Times to repeat the playback of 'play track'." ) );
    m_spin_waittime->setToolTip( i18n( "Time in seconds to wait after playback of 'play track'." ) );

    m_comboAfterTimeout->setWhatsThis( i18n( "<p>Target to be jumped to on time-out of <wait>."
                                             "<p>If omitted (and <wait> is not set to an infinite time) one of the targets is selected at random." ) );
    m_check_reactivity->setWhatsThis( i18n( "<p>When reactivity is set to delayed, it is recommended that the length of the referenced 'play track' is not more than 5 seconds."
                                            "<p>The recommended setting for a play item consisting of one still picture and no audio is to loop once and have a delayed reactivity." ) );
    m_check_pbc->setWhatsThis( i18n( "<p>Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats."
                                     "<p>PBC allows control of the playback of play items and the possibility of interaction with the user through the remote control or some other input device available." ) );
    m_check_usekeys->setWhatsThis( i18n( "These are actually pseudo keys, representing the numeric keys 0, 1, ..., 9." ) );
    m_check_overwritekeys->setWhatsThis( i18n( "<p>If numeric keys enabled, you can overwrite the default settings." ) );
    m_spin_times->setWhatsThis( i18n( "<p>Times to repeat the playback of 'play track'."
                                      "<p>The reactivity attribute controls whether the playback of 'play track' is finished, thus delayed, before executing user triggered action or an immediate jump is performed."
                                      "<p>After the specified number of repetitions have completed, the <wait> time begins to count down, unless set to an infinite wait time."
                                      "<p>If this element is omitted, a default of `1' is used, i.e. the 'play track' will be displayed once." ) );
    m_spin_waittime->setWhatsThis( i18n( "Time in seconds to wait after playback of 'play track' before triggering the <timeout> action (unless the user triggers some action before time ran up)." ) );

}

void K3b::VcdTrackDialog::fillPbcGui()
{
    K3b::VcdTrack * selectedTrack = m_selectedTracks.first();
    // add tracktitles to combobox
    int iPrevious = -1;
    int iNext = -1;
    int iReturn = -1;
    int iDefault = -1;
    int iAfterTimeOut = -1;

    Q_FOREACH( K3b::VcdTrack* track, m_tracks ) {
        QPixmap pm;
        if ( track->isSegment() )
            pm = SmallIcon( "image-x-generic" );
        else
            pm = SmallIcon( "video-x-generic" );

        QString s = VcdTrackKeysModel::trackName( track, selectedTrack );

        m_pbc_previous->addItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::PREVIOUS ) )
            iPrevious = m_pbc_previous->count() - 1;

        m_pbc_next->addItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::NEXT ) )
            iNext = m_pbc_next->count() - 1;

        m_pbc_return->addItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::RETURN ) )
            iReturn = m_pbc_return->count() - 1;

        m_pbc_default->addItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::DEFAULT ) )
            iDefault = m_pbc_default->count() - 1;

        m_comboAfterTimeout->addItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::AFTERTIMEOUT ) )
            iAfterTimeOut = m_comboAfterTimeout->count() - 1;

    }

    // add Event Disabled
    QPixmap pmDisabled = SmallIcon( "process-stop" );
    QString txtDisabled = i18n( "Event Disabled" );
    m_pbc_previous->addItem( pmDisabled, txtDisabled );
    m_pbc_next->addItem( pmDisabled, txtDisabled );
    m_pbc_return->addItem( pmDisabled, txtDisabled );
    m_pbc_default->addItem( pmDisabled, txtDisabled );
    m_comboAfterTimeout->addItem( pmDisabled, txtDisabled );

    // add VideoCD End
    QPixmap pmEnd = SmallIcon( "media-optical-video" );
    QString txtEnd = i18n( "VideoCD END" );
    m_pbc_previous->addItem( pmEnd, txtEnd );
    m_pbc_next->addItem( pmEnd, txtEnd );
    m_pbc_return->addItem( pmEnd, txtEnd );
    m_pbc_default->addItem( pmEnd, txtEnd );
    m_comboAfterTimeout->addItem( pmEnd, txtEnd );

    int count = m_tracks.count();

    if ( iPrevious < 0 )
        m_pbc_previous->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::PREVIOUS ) );
    else
        m_pbc_previous->setCurrentIndex( iPrevious );

    if ( iNext < 0 )
        m_pbc_next->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::NEXT ) );
    else
        m_pbc_next->setCurrentIndex( iNext );

    if ( iReturn < 0 )
        m_pbc_return->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::RETURN ) );
    else
        m_pbc_return->setCurrentIndex( iReturn );

    if ( iDefault < 0 )
        m_pbc_default->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::DEFAULT ) );
    else
        m_pbc_default->setCurrentIndex( iDefault );

    if ( iAfterTimeOut < 0 )
        m_comboAfterTimeout->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::AFTERTIMEOUT ) );
    else
        m_comboAfterTimeout->setCurrentIndex( iAfterTimeOut );


    m_spin_waittime->setValue( selectedTrack->getWaitTime() );
    m_spin_times->setValue( selectedTrack->getPlayTime() );

    m_check_reactivity->setChecked( selectedTrack->Reactivity() );
    m_check_pbc->setChecked( VcdOptions() ->PbcEnabled() );

    m_check_usekeys->setChecked( selectedTrack->PbcNumKeys() );
    m_check_overwritekeys->setChecked( selectedTrack->PbcNumKeysUserdefined() );

    m_mainTabbed->setTabEnabled( m_mainTabbed->indexOf( m_widgetnumkeys ),
                                 m_check_usekeys->isChecked() && m_check_pbc->isChecked() );
}

void K3b::VcdTrackDialog::prepareGui()
{
    QWidget * frame = mainWidget();

    QGridLayout* mainLayout = new QGridLayout( frame );
    mainLayout->setMargin( 0 );

    m_mainTabbed = new QTabWidget( frame );

    // /////////////////////////////////////////////////
    // FILE-INFO BOX
    // /////////////////////////////////////////////////
    QGroupBox* groupFileInfo = new QGroupBox( i18n( "File Info" ), frame );

    QGridLayout* groupFileInfoLayout = new QGridLayout( groupFileInfo );
    groupFileInfoLayout->setAlignment( Qt::AlignTop );

    m_labelMimeType = new QLabel( groupFileInfo );

    m_displayFileName = new KSqueezedTextLabel( groupFileInfo );
    m_displayFileName->setText( i18n( "Filename" ) );
    m_displayFileName->setAlignment( Qt::AlignTop | Qt::AlignLeft );

    QLabel* labelSize = new QLabel( i18n( "Size:" ), groupFileInfo );
    QLabel* labelLength = new QLabel( i18n( "Length:" ), groupFileInfo );
    QLabel* labelMuxrate = new QLabel( i18n( "Muxrate:" ), groupFileInfo );

    m_displaySize = new QLabel( groupFileInfo );
    m_displaySize->setText( "0.0 MB" );
    m_displaySize->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    m_displayLength = new QLabel( groupFileInfo );
    m_displayLength->setText( "0:0:0" );
    m_displayLength->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    m_muxrate = new QLabel( groupFileInfo );
    m_muxrate->setText( i18n( "%1 bit/s" ,QString::number( 0 ) ));
    m_muxrate->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    QFrame* fileInfoLine = new QFrame( groupFileInfo );
    fileInfoLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    groupFileInfoLayout->addWidget( m_labelMimeType, 0, 0 );
    groupFileInfoLayout->addWidget( m_displayFileName, 0, 1, 2, 1 );
    groupFileInfoLayout->addWidget( fileInfoLine, 2, 0, 1, 2 );
    groupFileInfoLayout->addWidget( labelLength, 3, 0 );
    groupFileInfoLayout->addWidget( labelSize, 4, 0 );
    groupFileInfoLayout->addWidget( labelMuxrate, 5, 0 );
    groupFileInfoLayout->addWidget( m_displayLength, 3, 1 );
    groupFileInfoLayout->addWidget( m_displaySize, 4, 1 );
    groupFileInfoLayout->addWidget( m_muxrate, 5, 1 );

    groupFileInfoLayout->setRowStretch( 6, 1 );
    groupFileInfoLayout->setColumnStretch( 1, 1 );

    QFont f( m_displayLength->font() );
    f.setBold( true );
    m_displayLength->setFont( f );
    m_displaySize->setFont( f );
    m_muxrate->setFont( f );
///////////////////////////////////////////////////

        mainLayout->addWidget( groupFileInfo, 0, 0 );
        mainLayout->addWidget( m_mainTabbed, 0, 1 );

        //  mainLayout->setColumnStretch( 0, 1 );

}

void K3b::VcdTrackDialog::setupPbcTab()
{
    // /////////////////////////////////////////////////
    // Playback Control TAB
    // /////////////////////////////////////////////////
    QWidget * w = new QWidget( m_mainTabbed );

    QGridLayout* grid = new QGridLayout( w );
    grid->setAlignment( Qt::AlignTop );


    //////////////////////////////////////////////////////////////////////////////////////////
    QGroupBox* groupOptions = new QGroupBox( i18n( "Settings" ), w );

    m_check_pbc = new QCheckBox( i18n( "Enable playback control (for the whole CD)" ), groupOptions );
    m_check_usekeys = new QCheckBox( i18n( "Use numeric keys" ), groupOptions );
    m_check_usekeys->setEnabled( false );
    m_check_reactivity = new QCheckBox( i18n( "Reactivity delayed to the end of playing track" ), groupOptions );
    m_check_reactivity->setEnabled( false );

    QVBoxLayout* groupOptionsLayout = new QVBoxLayout( groupOptions );
    groupOptionsLayout->addWidget( m_check_pbc );
    groupOptionsLayout->addWidget( m_check_usekeys );
    groupOptionsLayout->addWidget( m_check_reactivity );

    //////////////////////////////////////////////////////////////////////////////////////////
    m_groupPlay = new QGroupBox( i18n( "Playing" ), w );

    QGridLayout* groupPlayLayout = new QGridLayout( m_groupPlay );
    groupPlayLayout->setAlignment( Qt::AlignTop );

    QLabel* labelPlaying = new QLabel( i18n( "Playing track" ) , m_groupPlay );

    m_spin_times = new QSpinBox( m_groupPlay );
    m_spin_times->setValue( 1 );
    m_spin_times->setSuffix( i18n( " time(s)" ) );
    m_spin_times->setSpecialValueText( i18n( "forever" ) );

    //////////////////////////////////////////////////////////////////////////////////////////
    m_labelWait = new QLabel( i18n( "then wait" ), m_groupPlay );
    m_spin_waittime = new QSpinBox( m_groupPlay );
    m_spin_waittime->setMinimum( -1 );
    m_spin_waittime->setValue( 0 );
    // m_spin_waittime->setEnabled( false );
    m_spin_waittime->setSuffix( i18n( " seconds" ) );
    m_spin_waittime->setSpecialValueText( i18n( "infinite" ) );

    m_labelAfterTimeout = new QLabel( i18n( "after timeout playing" ), m_groupPlay );
    // m_labelAfterTimeout->setEnabled( false );
    m_comboAfterTimeout = new K3b::CutComboBox( /*K3b::CutComboBox::SQUEEZE, */m_groupPlay );
    // m_comboAfterTimeout->setEnabled( false );

    groupPlayLayout->addWidget( labelPlaying, 1, 0 );
    groupPlayLayout->addWidget( m_spin_times, 1, 1 );
    groupPlayLayout->addWidget( m_labelWait, 1, 2 );
    groupPlayLayout->addWidget( m_spin_waittime, 1, 3 );
    groupPlayLayout->addWidget( m_labelAfterTimeout, 2, 1, 1, 3 );
    groupPlayLayout->addWidget( m_comboAfterTimeout, 3, 1, 1, 3 );

    //////////////////////////////////////////////////////////////////////////////////////////
    m_groupPbc = new QGroupBox( i18n( "Key Pressed Interaction" ), w );

    QGridLayout* groupPbcLayout = new QGridLayout( m_groupPbc );
    groupPbcLayout->setAlignment( Qt::AlignTop );

    QLabel* labelPbc_previous = new QLabel( i18n( "Previous:" ), m_groupPbc );
    QLabel* labelPbc_next = new QLabel( i18n( "Next:" ), m_groupPbc );
    QLabel* labelPbc_return = new QLabel( i18n( "Return:" ), m_groupPbc );
    QLabel* labelPbc_default = new QLabel( i18n( "Default:" ), m_groupPbc );

    m_pbc_previous = new K3b::CutComboBox( /*K3b::CutComboBox::SQUEEZE,*/ m_groupPbc );
    m_pbc_next = new K3b::CutComboBox( /*K3b::CutComboBox::SQUEEZE,*/ m_groupPbc );
    m_pbc_return = new K3b::CutComboBox( /*K3b::CutComboBox::SQUEEZE,*/ m_groupPbc );
    m_pbc_default = new K3b::CutComboBox( /*K3b::CutComboBox::SQUEEZE,*/ m_groupPbc );

    groupPbcLayout->addWidget( labelPbc_previous, 1, 0 );
    groupPbcLayout->addWidget( m_pbc_previous, 1, 1, 1, 3 );

    groupPbcLayout->addWidget( labelPbc_next, 2, 0 );
    groupPbcLayout->addWidget( m_pbc_next, 2, 1, 1, 3 );

    groupPbcLayout->addWidget( labelPbc_return, 3, 0 );
    groupPbcLayout->addWidget( m_pbc_return, 3, 1, 1, 3 );

    groupPbcLayout->addWidget( labelPbc_default, 4, 0 );
    groupPbcLayout->addWidget( m_pbc_default, 4, 1, 1, 3 );


    grid->addWidget( groupOptions, 0, 0 );
    grid->addWidget( m_groupPlay, 1, 0 );
    grid->addWidget( m_groupPbc, 2, 0 );

    grid->setRowStretch( 9, 1 );

    m_mainTabbed->addTab( w, i18n( "Playback Control" ) );

    m_groupPlay->setEnabled( false );
    m_groupPbc->setEnabled( false );

    connect( m_check_pbc, SIGNAL( toggled( bool ) ), this, SLOT( slotPbcToggled( bool ) ) );
    connect( m_spin_times, SIGNAL( valueChanged( int ) ), this, SLOT( slotPlayTimeChanged( int ) ) );
    connect( m_spin_waittime, SIGNAL( valueChanged( int ) ), this, SLOT( slotWaitTimeChanged( int ) ) );
    connect( m_check_usekeys, SIGNAL( toggled( bool ) ), this, SLOT( slotUseKeysToggled( bool ) ) );
}

void K3b::VcdTrackDialog::setupPbcKeyTab()
{
    // /////////////////////////////////////////////////
    // Playback Control Numeric Key's TAB
    // /////////////////////////////////////////////////
    m_widgetnumkeys = new QWidget( m_mainTabbed );

    QGridLayout* grid = new QGridLayout( m_widgetnumkeys );
    grid->setAlignment( Qt::AlignTop );

    m_groupKey = new QGroupBox( i18n( "Numeric Keys" ), m_widgetnumkeys );
    m_groupKey->setEnabled( false );
    
    m_keys_model = new VcdTrackKeysModel( m_selectedTracks.first(), 99, this );
    m_keys_delegate = new VcdTrackKeysDelegate( m_tracks, m_selectedTracks.first(), this );

    m_keys_view = new QTreeView( m_groupKey );
    m_keys_view->setModel( m_keys_model );
    m_keys_view->setItemDelegateForColumn( VcdTrackKeysModel::PlayingColumn, m_keys_delegate );
    m_keys_view->setAllColumnsShowFocus( true );
    m_keys_view->setRootIsDecorated( false );
    m_keys_view->setEditTriggers( QAbstractItemView::AllEditTriggers );
    m_keys_view->header()->setResizeMode( VcdTrackKeysModel::KeyColumn, QHeaderView::ResizeToContents );

    QVBoxLayout* groupKeyLayout = new QVBoxLayout( m_groupKey );
    groupKeyLayout->addWidget( m_keys_view );

    m_check_overwritekeys = new QCheckBox( i18n( "Overwrite default assignment" ), m_widgetnumkeys );

    grid->addWidget( m_groupKey, 1, 0 );
    grid->addWidget( m_check_overwritekeys, 2, 0 );

    m_mainTabbed->addTab( m_widgetnumkeys, i18n( "Numeric Keys" ) );

    connect( m_check_overwritekeys, SIGNAL(toggled(bool)), m_groupKey, SLOT(setEnabled(bool)) );
}

void K3b::VcdTrackDialog::setupAudioTab()
{
    // /////////////////////////////////////////////////
    // AUDIO TAB
    // /////////////////////////////////////////////////
    QWidget * w = new QWidget( m_mainTabbed );

    QGridLayout* grid = new QGridLayout( w );
    grid->setAlignment( Qt::AlignTop );

    QLabel* labelMpegVer_Audio = new QLabel( i18n( "Type:" ), w );
    QLabel* labelRate_Audio = new QLabel( i18n( "Rate:" ), w );
    QLabel* labelSampling_Frequency_Audio = new QLabel( i18n( "Sampling frequency:" ), w );
    QLabel* labelMode_Audio = new QLabel( i18n( "Mode:" ), w );
    QLabel* labelCopyright_Audio = new QLabel( i18n( "Copyright:" ), w );

    m_mpegver_audio = new QLabel( w );
    m_rate_audio = new QLabel( w );
    m_sampling_frequency_audio = new QLabel( w );
    m_mode_audio = new QLabel( w );
    m_copyright_audio = new QLabel( w );

    m_mpegver_audio->setFrameShape( QLabel::LineEditPanel );
    m_rate_audio->setFrameShape( QLabel::LineEditPanel );
    m_sampling_frequency_audio->setFrameShape( QLabel::LineEditPanel );
    m_mode_audio->setFrameShape( QLabel::LineEditPanel );
    m_copyright_audio->setFrameShape( QLabel::LineEditPanel );

    m_mpegver_audio->setFrameShadow( QLabel::Sunken );
    m_rate_audio->setFrameShadow( QLabel::Sunken );
    m_sampling_frequency_audio->setFrameShadow( QLabel::Sunken );
    m_mode_audio->setFrameShadow( QLabel::Sunken );
    m_copyright_audio->setFrameShadow( QLabel::Sunken );

    grid->addWidget( labelMpegVer_Audio, 1, 0 );
    grid->addWidget( m_mpegver_audio, 1, 1, 1, 4 );

    grid->addWidget( labelRate_Audio, 2, 0 );
    grid->addWidget( m_rate_audio, 2, 1, 1, 4 );

    grid->addWidget( labelSampling_Frequency_Audio, 3, 0 );
    grid->addWidget( m_sampling_frequency_audio, 3, 1, 1, 4 );

    grid->addWidget( labelMode_Audio, 4, 0 );
    grid->addWidget( m_mode_audio, 4, 1, 1, 4 );

    grid->addWidget( labelCopyright_Audio, 5, 0 );
    grid->addWidget( m_copyright_audio, 5, 1, 1, 4 );

    grid->setRowStretch( 9, 4 );

    m_mainTabbed->addTab( w, i18n( "Audio" ) );

}

void K3b::VcdTrackDialog::setupVideoTab()
{
    // /////////////////////////////////////////////////
    // VIDEO TAB
    // /////////////////////////////////////////////////
    QWidget * w = new QWidget( m_mainTabbed );

    QGridLayout* grid = new QGridLayout( w );
    grid->setAlignment( Qt::AlignTop );

    QLabel* labelMpegVer_Video = new QLabel( i18n( "Type:" ), w );
    QLabel* labelRate_Video = new QLabel( i18n( "Rate:" ), w );
    QLabel* labelChromaFormat_Video = new QLabel( i18n( "Chroma format:" ), w );
    QLabel* labelFormat_Video = new QLabel( i18n( "Video format:" ), w );
    QLabel* labelResolution_Video = new QLabel( i18n( "Resolution:" ), w );
    QLabel* labelHighResolution_Video = new QLabel( i18n( "High resolution:" ), w );

    m_mpegver_video = new QLabel( w );
    m_rate_video = new QLabel( w );
    m_chromaformat_video = new QLabel( w );
    m_format_video = new QLabel( w );
    m_resolution_video = new QLabel( w );
    m_highresolution_video = new QLabel( w );

    m_mpegver_video->setFrameShape( QLabel::LineEditPanel );
    m_rate_video->setFrameShape( QLabel::LineEditPanel );
    m_chromaformat_video->setFrameShape( QLabel::LineEditPanel );
    m_format_video->setFrameShape( QLabel::LineEditPanel );
    m_resolution_video->setFrameShape( QLabel::LineEditPanel );
    m_highresolution_video->setFrameShape( QLabel::LineEditPanel );

    m_mpegver_video->setFrameShadow( QLabel::Sunken );
    m_rate_video->setFrameShadow( QLabel::Sunken );
    m_chromaformat_video->setFrameShadow( QLabel::Sunken );
    m_format_video->setFrameShadow( QLabel::Sunken );
    m_resolution_video->setFrameShadow( QLabel::Sunken );
    m_highresolution_video->setFrameShadow( QLabel::Sunken );

    grid->addWidget( labelMpegVer_Video, 1, 0 );
    grid->addWidget( m_mpegver_video, 1, 1, 1, 4 );

    grid->addWidget( labelRate_Video, 2, 0 );
    grid->addWidget( m_rate_video, 2, 1, 1, 4 );

    grid->addWidget( labelChromaFormat_Video, 3, 0 );
    grid->addWidget( m_chromaformat_video, 3, 1, 1, 4 );

    grid->addWidget( labelFormat_Video, 4, 0 );
    grid->addWidget( m_format_video, 4, 1, 1, 4 );

    grid->addWidget( labelResolution_Video, 5, 0 );
    grid->addWidget( m_resolution_video, 5, 1, 1, 4 );

    grid->addWidget( labelHighResolution_Video, 6, 0 );
    grid->addWidget( m_highresolution_video, 6, 1, 1, 4 );

    grid->setRowStretch( 9, 4 );

    m_mainTabbed->addTab( w, i18n( "Video" ) );
}

void K3b::VcdTrackDialog::slotPlayTimeChanged( int value )
{
    if ( value == 0 ) {
        m_labelWait->setEnabled( false );
        m_spin_waittime->setEnabled( false );
        m_labelAfterTimeout->setEnabled( false );
        m_comboAfterTimeout->setEnabled( false );
    } else {
        m_labelWait->setEnabled( true );
        m_spin_waittime->setEnabled( true );
        if ( m_spin_waittime->value() > -1 ) {
            m_labelAfterTimeout->setEnabled( true );
            m_comboAfterTimeout->setEnabled( true );
        }
    }
}

void K3b::VcdTrackDialog::slotWaitTimeChanged( int value )
{
    if ( value < 0 || !m_labelWait->isEnabled() ) {
        m_labelAfterTimeout->setEnabled( false );
        m_comboAfterTimeout->setEnabled( false );
    } else {
        m_labelAfterTimeout->setEnabled( true );
        m_comboAfterTimeout->setEnabled( true );
    }
}

void K3b::VcdTrackDialog::slotPbcToggled( bool b )
{
    m_groupPlay->setEnabled( b );
    m_groupPbc->setEnabled( b );
    m_check_usekeys->setEnabled( b );
    slotUseKeysToggled( b && m_check_usekeys->isChecked() );
    m_check_reactivity->setEnabled( b );
    if ( b )
        slotWaitTimeChanged( m_spin_waittime->value() );
}

void K3b::VcdTrackDialog::slotUseKeysToggled( bool b )
{
    m_mainTabbed->setTabEnabled( m_mainTabbed->indexOf( m_widgetnumkeys ), b );
}

#include "k3bvcdtrackdialog.moc"
