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

#include "k3bvcdtrackdialog.h"
#include "k3bglobals.h"
#include "k3bmsf.h"
#include "k3bvcddoc.h"
#include "k3bvcdtrack.h"
#include "k3bvcdtrackkeysdelegate.h"
#include "k3bvcdtrackkeysmodel.h"

#include <KIconLoader>
#include <KIO/Global>
#include <KLocalizedString>
#include <KSqueezedTextLabel>

#include <QList>
#include <QUrl>
#include <QPixmap>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolTip>
#include <QTreeView>
#include <QVBoxLayout>


namespace {
    const int KEY_COUNT = 99;
} // namespace


class K3b::VcdTrackDialog::Private
{
public:
    Private( K3b::VcdDoc* d, const QList<K3b::VcdTrack*>& t, QList<K3b::VcdTrack*>& st )
        : doc( d ), tracks( t ), selectedTracks( st )
    {
    }

    void setPbcTrack( VcdTrack* selected, QComboBox* box, VcdTrack::PbcTracks which );

    VcdDoc* doc;
    QList<VcdTrack*> tracks;
    QList<VcdTrack*> selectedTracks;
    QTabWidget* mainTabbed;

    KSqueezedTextLabel* displayFileName;
    QLabel* labelMimeType;
    QLabel* displaySize;
    QLabel* displayLength;
    QLabel* muxrate;

    QLabel* mpegver_audio;
    QLabel* rate_audio;
    QLabel* sampling_frequency_audio;
    QLabel* mode_audio;
    QLabel* copyright_audio;

    QLabel* mpegver_video;
    QLabel* rate_video;
    QLabel* chromaformat_video;
    QLabel* format_video;
    QLabel* resolution_video;
    QLabel* highresolution_video;

    QLabel* labelAfterTimeout;
    QLabel* labelWait;

    QGroupBox* groupPlay;
    QGroupBox* groupPbc;
    QWidget* widgetnumkeys;

    QComboBox* pbc_previous;
    QComboBox* pbc_next;
    QComboBox* pbc_return;
    QComboBox* pbc_default;
    QComboBox* comboAfterTimeout;

    QCheckBox* check_reactivity;
    QCheckBox* check_pbc;
    QCheckBox* check_usekeys;
    QCheckBox* check_overwritekeys;
    QTreeView* keys_view;
    VcdTrackKeysModel* keys_model;
    VcdTrackKeysDelegate* keys_delegate;

    QSpinBox* spin_times;
    QSpinBox* spin_waittime;
};


void K3b::VcdTrackDialog::Private::setPbcTrack( K3b::VcdTrack* selected, QComboBox* box, VcdTrack::PbcTracks which )
{
    const int currentIndex = box->currentIndex();
    const int count = tracks.count();

    // TODO: Unset Userdefined on default settings
    qDebug() << QString( "K3b::VcdTrackDialog::setPbcTrack: currentIndex = %1, count = %2" ).arg( currentIndex ).arg( count );

    if( VcdTrack* track = selected->getPbcTrack( which ) )
        track->delFromRevRefList( selected );

    if( currentIndex >= 0 && currentIndex < count ) {
        selected->setPbcTrack( which, tracks.at( currentIndex ) );
        tracks.at( currentIndex ) ->addToRevRefList( selected );
    }
    else if( currentIndex == count ) {
        selected->setPbcTrack( which, 0 );
        selected->setPbcNonTrack( which, VcdTrack::DISABLED );
    }
    else {
        selected->setPbcTrack( which, 0 );
        selected->setPbcNonTrack( which, VcdTrack::VIDEOEND );
    }

    selected->setUserDefined( which, true );
}


K3b::VcdTrackDialog::VcdTrackDialog( K3b::VcdDoc* doc, const QList<K3b::VcdTrack*>& tracks, QList<K3b::VcdTrack*>& selectedTracks, QWidget* parent )
    : QDialog( parent ),
      d( new Private( doc, tracks, selectedTracks ) )
{
    setWindowTitle( i18n( "Video Track Properties" ) );

    prepareGui();

    setupPbcTab();
    setupPbcKeyTab();
    setupVideoTab();
    setupAudioTab();

    if ( !d->selectedTracks.isEmpty() ) {

        K3b::VcdTrack * selectedTrack = d->selectedTracks.first();

        d->displayFileName->setText( selectedTrack->fileName() );
        d->displayLength->setText( selectedTrack->duration() );
        d->displaySize->setText( KIO::convertSize( selectedTrack->size() ) );
        d->muxrate->setText( i18n( "%1 bit/s", selectedTrack->muxrate() ) );

        if ( selectedTrack->isSegment() )
            d->labelMimeType->setPixmap( SmallIcon( "image-x-generic", KIconLoader::SizeMedium ) );
        else
            d->labelMimeType->setPixmap( SmallIcon( "video-x-generic", KIconLoader::SizeMedium ) );

        fillGui();
    }
}

K3b::VcdTrackDialog::~VcdTrackDialog()
{
    delete d;
}

void K3b::VcdTrackDialog::accept()
{
    slotApply();
    QDialog::accept();
}

void K3b::VcdTrackDialog::slotApply()
{
    // track set
    K3b::VcdTrack * selectedTrack = d->selectedTracks.first();

    d->setPbcTrack( selectedTrack, d->pbc_previous, K3b::VcdTrack::PREVIOUS );
    d->setPbcTrack( selectedTrack, d->pbc_next, K3b::VcdTrack::NEXT );
    d->setPbcTrack( selectedTrack, d->pbc_return, K3b::VcdTrack::RETURN );
    d->setPbcTrack( selectedTrack, d->pbc_default, K3b::VcdTrack::DEFAULT );
    d->setPbcTrack( selectedTrack, d->comboAfterTimeout, K3b::VcdTrack::AFTERTIMEOUT );

    selectedTrack->setPlayTime( d->spin_times->value() );
    selectedTrack->setWaitTime( d->spin_waittime->value() );
    selectedTrack->setReactivity( d->check_reactivity->isChecked() );
    selectedTrack->setPbcNumKeys( d->check_usekeys->isChecked() );
    selectedTrack->setPbcNumKeysUserdefined( d->check_overwritekeys->isChecked() );

    // global set
    d->doc->vcdOptions()->setPbcEnabled( d->check_pbc->isChecked() );

    // define numeric keys
    selectedTrack->delDefinedNumKey();

    if ( d->check_overwritekeys->isChecked() ) {

        for( int key = 1; key <= d->keys_model->keyCount(); ++key ) {
            VcdTrackKeysModel::Key2Track::const_iterator it = d->keys_model->keys().constFind( key );
            if( it != d->keys_model->keys().constEnd() )
                selectedTrack->setDefinedNumKey( it.key(), it.value() );
            else
                selectedTrack->delDefinedNumKey( it.key() );
        }
    } else {
        selectedTrack->setDefinedNumKey( 1, selectedTrack );
        qDebug() << "Key 1" << " Playing: (default) " << VcdTrackKeysModel::trackName( selectedTrack ) << "Track: " << selectedTrack;
    }
}

void K3b::VcdTrackDialog::fillGui()
{
    K3b::VcdTrack * selectedTrack = d->selectedTracks.first();

    d->mpegver_video->setText( selectedTrack->mpegTypeS() );
    d->rate_video->setText( selectedTrack->video_bitrate() );
    d->chromaformat_video->setText( selectedTrack->video_chroma() );
    d->format_video->setText( selectedTrack->video_format() );
    d->highresolution_video->setText( selectedTrack->highresolution() );
    d->resolution_video->setText( selectedTrack->resolution() );

    d->mpegver_audio->setText( selectedTrack->mpegTypeS( true ) );
    d->rate_audio->setText( selectedTrack->audio_bitrate() );

    d->sampling_frequency_audio->setText( selectedTrack->audio_sampfreq() );
    d->mode_audio->setText( selectedTrack->audio_mode() );
    d->copyright_audio->setText( selectedTrack->audio_copyright() );

    fillPbcGui();


    d->pbc_previous->setToolTip( i18n( "May also look like | << on the remote control. " ) );
    d->pbc_next->setToolTip( i18n( "May also look like >> | on the remote control." ) );
    d->pbc_return->setToolTip( i18n( "This key may be mapped to the STOP key." ) );
    d->pbc_default->setToolTip( i18n( "This key is usually mapped to the > or PLAY key." ) );
    d->comboAfterTimeout->setToolTip( i18n( "Target to be jumped to on time-out of <wait>." ) );
    d->check_reactivity->setToolTip( i18n( "Delay reactivity of keys." ) );
    d->check_pbc->setToolTip( i18n( "Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats." ) );
    d->check_usekeys->setToolTip( i18n( "Activate the use of numeric keys." ) );
    d->check_overwritekeys->setToolTip( i18n( "Overwrite default numeric keys." ) );
    d->keys_view->setToolTip( i18n( "Numeric keys." ) );
    d->spin_times->setToolTip( i18n( "Times to repeat the playback of 'play track'." ) );
    d->spin_waittime->setToolTip( i18n( "Time in seconds to wait after playback of 'play track'." ) );

    d->comboAfterTimeout->setWhatsThis( i18n( "<p>Target to be jumped to on time-out of <wait>."
                                             "<p>If omitted (and <wait> is not set to an infinite time) one of the targets is selected at random." ) );
    d->check_reactivity->setWhatsThis( i18n( "<p>When reactivity is set to delayed, it is recommended that the length of the referenced 'play track' is not more than 5 seconds."
                                            "<p>The recommended setting for a play item consisting of one still picture and no audio is to loop once and have a delayed reactivity." ) );
    d->check_pbc->setWhatsThis( i18n( "<p>Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats."
                                     "<p>PBC allows control of the playback of play items and the possibility of interaction with the user through the remote control or some other input device available." ) );
    d->check_usekeys->setWhatsThis( i18n( "These are actually pseudo keys, representing the numeric keys 0, 1, ..., 9." ) );
    d->check_overwritekeys->setWhatsThis( i18n( "<p>If numeric keys enabled, you can overwrite the default settings." ) );
    d->spin_times->setWhatsThis( i18n( "<p>Times to repeat the playback of 'play track'."
                                      "<p>The reactivity attribute controls whether the playback of 'play track' is finished, thus delayed, before executing user triggered action or an immediate jump is performed."
                                      "<p>After the specified number of repetitions have completed, the <wait> time begins to count down, unless set to an infinite wait time."
                                      "<p>If this element is omitted, a default of `1' is used, i.e. the 'play track' will be displayed once." ) );
    d->spin_waittime->setWhatsThis( i18n( "Time in seconds to wait after playback of 'play track' before triggering the <timeout> action (unless the user triggers some action before time ran up)." ) );

}

void K3b::VcdTrackDialog::fillPbcGui()
{
    K3b::VcdTrack * selectedTrack = d->selectedTracks.first();
    // add tracktitles to combobox
    int iPrevious = -1;
    int iNext = -1;
    int iReturn = -1;
    int iDefault = -1;
    int iAfterTimeOut = -1;

    Q_FOREACH( K3b::VcdTrack* track, d->tracks ) {
        QString name = VcdTrackKeysModel::trackName( track );
        QIcon icon = VcdTrackKeysModel::trackIcon( track );

        d->pbc_previous->addItem( icon, name );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::PREVIOUS ) )
            iPrevious = d->pbc_previous->count() - 1;

        d->pbc_next->addItem( icon, name );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::NEXT ) )
            iNext = d->pbc_next->count() - 1;

        d->pbc_return->addItem( icon, name );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::RETURN ) )
            iReturn = d->pbc_return->count() - 1;

        d->pbc_default->addItem( icon, name );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::DEFAULT ) )
            iDefault = d->pbc_default->count() - 1;

        d->comboAfterTimeout->addItem( icon, name );
        if ( track == selectedTrack->getPbcTrack( K3b::VcdTrack::AFTERTIMEOUT ) )
            iAfterTimeOut = d->comboAfterTimeout->count() - 1;
    }

    // add Event Disabled
    QPixmap disabledIcon = SmallIcon( "process-stop" );
    QString disabledName = i18n( "Event Disabled" );
    d->pbc_previous->addItem( disabledIcon, disabledName );
    d->pbc_next->addItem( disabledIcon, disabledName );
    d->pbc_return->addItem( disabledIcon, disabledName );
    d->pbc_default->addItem( disabledIcon, disabledName );
    d->comboAfterTimeout->addItem( disabledIcon, disabledName );

    // add VideoCD End
    QString endName = VcdTrackKeysModel::trackName( 0 );
    QIcon endIcon = VcdTrackKeysModel::trackIcon( 0 );
    d->pbc_previous->addItem( endIcon, endName );
    d->pbc_next->addItem( endIcon, endName );
    d->pbc_return->addItem( endIcon, endName );
    d->pbc_default->addItem( endIcon, endName );
    d->comboAfterTimeout->addItem( endIcon, endName );

    int count = d->tracks.count();

    if ( iPrevious < 0 )
        d->pbc_previous->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::PREVIOUS ) );
    else
        d->pbc_previous->setCurrentIndex( iPrevious );

    if ( iNext < 0 )
        d->pbc_next->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::NEXT ) );
    else
        d->pbc_next->setCurrentIndex( iNext );

    if ( iReturn < 0 )
        d->pbc_return->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::RETURN ) );
    else
        d->pbc_return->setCurrentIndex( iReturn );

    if ( iDefault < 0 )
        d->pbc_default->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::DEFAULT ) );
    else
        d->pbc_default->setCurrentIndex( iDefault );

    if ( iAfterTimeOut < 0 )
        d->comboAfterTimeout->setCurrentIndex( count + selectedTrack->getNonPbcTrack( K3b::VcdTrack::AFTERTIMEOUT ) );
    else
        d->comboAfterTimeout->setCurrentIndex( iAfterTimeOut );


    d->spin_waittime->setValue( selectedTrack->getWaitTime() );
    d->spin_times->setValue( selectedTrack->getPlayTime() );

    d->check_reactivity->setChecked( selectedTrack->Reactivity() );
    d->check_pbc->setChecked( d->doc->vcdOptions()->PbcEnabled() );

    d->check_usekeys->setChecked( selectedTrack->PbcNumKeys() );
    d->check_overwritekeys->setChecked( selectedTrack->PbcNumKeysUserdefined() );

    if( selectedTrack->PbcNumKeysUserdefined() ) {
        d->keys_model->setKeys( selectedTrack->DefinedNumKey() );
    }
    else {
        QMap<int, VcdTrack*> keys;
        keys.insert( 1, selectedTrack );
        d->keys_model->setKeys( keys );
    }

    d->mainTabbed->setTabEnabled( d->mainTabbed->indexOf( d->widgetnumkeys ),
                                 d->check_usekeys->isChecked() && d->check_pbc->isChecked() );
}

void K3b::VcdTrackDialog::prepareGui()
{
    // /////////////////////////////////////////////////
    // FILE-INFO BOX
    // /////////////////////////////////////////////////
    QGroupBox* groupFileInfo = new QGroupBox( i18n( "File Info" ), this );

    QGridLayout* groupFileInfoLayout = new QGridLayout( groupFileInfo );
    groupFileInfoLayout->setAlignment( Qt::AlignTop );

    d->labelMimeType = new QLabel( groupFileInfo );

    d->displayFileName = new KSqueezedTextLabel( groupFileInfo );
    d->displayFileName->setText( i18n( "Filename" ) );
    d->displayFileName->setAlignment( Qt::AlignTop | Qt::AlignLeft );

    QLabel* labelSize = new QLabel( i18n( "Size:" ), groupFileInfo );
    QLabel* labelLength = new QLabel( i18n( "Length:" ), groupFileInfo );
    QLabel* labelMuxrate = new QLabel( i18n( "Muxrate:" ), groupFileInfo );

    d->displaySize = new QLabel( groupFileInfo );
    d->displaySize->setText( "0.0 MB" );
    d->displaySize->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    d->displayLength = new QLabel( groupFileInfo );
    d->displayLength->setText( "0:0:0" );
    d->displayLength->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    d->muxrate = new QLabel( groupFileInfo );
    d->muxrate->setText( i18n( "%1 bit/s" ,QString::number( 0 ) ));
    d->muxrate->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

    QFrame* fileInfoLine = new QFrame( groupFileInfo );
    fileInfoLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    groupFileInfoLayout->addWidget( d->labelMimeType, 0, 0 );
    groupFileInfoLayout->addWidget( d->displayFileName, 0, 1, 2, 1 );
    groupFileInfoLayout->addWidget( fileInfoLine, 2, 0, 1, 2 );
    groupFileInfoLayout->addWidget( labelLength, 3, 0 );
    groupFileInfoLayout->addWidget( labelSize, 4, 0 );
    groupFileInfoLayout->addWidget( labelMuxrate, 5, 0 );
    groupFileInfoLayout->addWidget( d->displayLength, 3, 1 );
    groupFileInfoLayout->addWidget( d->displaySize, 4, 1 );
    groupFileInfoLayout->addWidget( d->muxrate, 5, 1 );

    groupFileInfoLayout->setRowStretch( 6, 1 );
    groupFileInfoLayout->setColumnStretch( 1, 1 );

    QFont f( d->displayLength->font() );
    f.setBold( true );
    d->displayLength->setFont( f );
    d->displaySize->setFont( f );
    d->muxrate->setFont( f );

    d->mainTabbed = new QTabWidget( this );

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this );
    connect( buttonBox, SIGNAL(accepted()), SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), SLOT(reject()) );
    connect( buttonBox->button( QDialogButtonBox::Apply ), SIGNAL(clicked()), SLOT(slotApply()) );

    QGridLayout* mainLayout = new QGridLayout( this );
    mainLayout->addWidget( groupFileInfo, 0, 0 );
    mainLayout->addWidget( d->mainTabbed, 0, 1 );
    mainLayout->addWidget( buttonBox, 1, 0, 1, 2 );
}

void K3b::VcdTrackDialog::setupPbcTab()
{
    // /////////////////////////////////////////////////
    // Playback Control TAB
    // /////////////////////////////////////////////////
    QWidget * w = new QWidget( d->mainTabbed );

    //////////////////////////////////////////////////////////////////////////////////////////
    QGroupBox* groupOptions = new QGroupBox( i18n( "Settings" ), w );

    d->check_pbc = new QCheckBox( i18n( "Enable playback control (for the whole CD)" ), groupOptions );
    d->check_usekeys = new QCheckBox( i18n( "Use numeric keys" ), groupOptions );
    d->check_usekeys->setEnabled( false );
    d->check_reactivity = new QCheckBox( i18n( "Reactivity delayed to the end of playing track" ), groupOptions );
    d->check_reactivity->setEnabled( false );

    QVBoxLayout* groupOptionsLayout = new QVBoxLayout( groupOptions );
    groupOptionsLayout->addWidget( d->check_pbc );
    groupOptionsLayout->addWidget( d->check_usekeys );
    groupOptionsLayout->addWidget( d->check_reactivity );

    //////////////////////////////////////////////////////////////////////////////////////////
    d->groupPlay = new QGroupBox( i18n( "Playing" ), w );

    QGridLayout* groupPlayLayout = new QGridLayout( d->groupPlay );
    groupPlayLayout->setAlignment( Qt::AlignTop );

    QLabel* labelPlaying = new QLabel( i18n( "Playing track" ) , d->groupPlay );

    d->spin_times = new QSpinBox( d->groupPlay );
    d->spin_times->setValue( 1 );
    d->spin_times->setSuffix( i18n( " time(s)" ) );
    d->spin_times->setSpecialValueText( i18n( "forever" ) );

    //////////////////////////////////////////////////////////////////////////////////////////
    d->labelWait = new QLabel( i18n( "then wait" ), d->groupPlay );
    d->spin_waittime = new QSpinBox( d->groupPlay );
    d->spin_waittime->setMinimum( -1 );
    d->spin_waittime->setValue( 0 );
    // d->spin_waittime->setEnabled( false );
    d->spin_waittime->setSuffix( i18n( " seconds" ) );
    d->spin_waittime->setSpecialValueText( i18n( "infinite" ) );

    d->labelAfterTimeout = new QLabel( i18n( "after timeout playing" ), d->groupPlay );
    // d->labelAfterTimeout->setEnabled( false );
    d->comboAfterTimeout = new QComboBox( /*K3b::CutComboBox::SQUEEZE, */d->groupPlay );
    // d->comboAfterTimeout->setEnabled( false );

    groupPlayLayout->addWidget( labelPlaying, 1, 0 );
    groupPlayLayout->addWidget( d->spin_times, 1, 1 );
    groupPlayLayout->addWidget( d->labelWait, 1, 2 );
    groupPlayLayout->addWidget( d->spin_waittime, 1, 3 );
    groupPlayLayout->addWidget( d->labelAfterTimeout, 2, 1, 1, 3 );
    groupPlayLayout->addWidget( d->comboAfterTimeout, 3, 1, 1, 3 );
    groupPlayLayout->setColumnStretch( 1, 1 );
    groupPlayLayout->setColumnStretch( 3, 1 );

    //////////////////////////////////////////////////////////////////////////////////////////
    d->groupPbc = new QGroupBox( i18n( "Key Pressed Interaction" ), w );

    QLabel* labelPbc_previous = new QLabel( i18n( "Previous:" ), d->groupPbc );
    QLabel* labelPbc_next = new QLabel( i18n( "Next:" ), d->groupPbc );
    QLabel* labelPbc_return = new QLabel( i18n( "Return:" ), d->groupPbc );
    QLabel* labelPbc_default = new QLabel( i18n( "Default:" ), d->groupPbc );

    d->pbc_previous = new QComboBox( /*K3b::CutComboBox::SQUEEZE,*/ d->groupPbc );
    d->pbc_next = new QComboBox( /*K3b::CutComboBox::SQUEEZE,*/ d->groupPbc );
    d->pbc_return = new QComboBox( /*K3b::CutComboBox::SQUEEZE,*/ d->groupPbc );
    d->pbc_default = new QComboBox( /*K3b::CutComboBox::SQUEEZE,*/ d->groupPbc );

    QFormLayout* groupPbcLayout = new QFormLayout( d->groupPbc );
    groupPbcLayout->setAlignment( Qt::AlignTop );
    groupPbcLayout->setFieldGrowthPolicy( QFormLayout::AllNonFixedFieldsGrow );
    groupPbcLayout->addRow( labelPbc_previous, d->pbc_previous );
    groupPbcLayout->addRow( labelPbc_next, d->pbc_next );
    groupPbcLayout->addRow( labelPbc_return, d->pbc_return );
    groupPbcLayout->addRow( labelPbc_default, d->pbc_default );

    QVBoxLayout* grid = new QVBoxLayout( w );
    grid->setAlignment( Qt::AlignTop );
    grid->addWidget( groupOptions );
    grid->addWidget( d->groupPlay );
    grid->addWidget( d->groupPbc );

    d->mainTabbed->addTab( w, i18n( "Playback Control" ) );

    d->groupPlay->setEnabled( false );
    d->groupPbc->setEnabled( false );

    connect( d->check_pbc, SIGNAL(toggled(bool)), this, SLOT(slotPbcToggled(bool)) );
    connect( d->spin_times, SIGNAL(valueChanged(int)), this, SLOT(slotPlayTimeChanged(int)) );
    connect( d->spin_waittime, SIGNAL(valueChanged(int)), this, SLOT(slotWaitTimeChanged(int)) );
    connect( d->check_usekeys, SIGNAL(toggled(bool)), this, SLOT(slotUseKeysToggled(bool)) );
}

void K3b::VcdTrackDialog::setupPbcKeyTab()
{
    // /////////////////////////////////////////////////
    // Playback Control Numeric Key's TAB
    // /////////////////////////////////////////////////
    d->widgetnumkeys = new QWidget( d->mainTabbed );

    d->check_overwritekeys = new QCheckBox( i18n( "Overwrite default assignment" ), d->widgetnumkeys );

    d->keys_model = new VcdTrackKeysModel( KEY_COUNT, this );
    d->keys_delegate = new VcdTrackKeysDelegate( d->tracks, this );

    d->keys_view = new QTreeView( d->widgetnumkeys );
    d->keys_view->setEnabled( false );
    d->keys_view->setModel( d->keys_model );
    d->keys_view->setItemDelegateForColumn( VcdTrackKeysModel::PlayingColumn, d->keys_delegate );
    d->keys_view->setAllColumnsShowFocus( true );
    d->keys_view->setRootIsDecorated( false );
    d->keys_view->setEditTriggers( QAbstractItemView::AllEditTriggers );
    d->keys_view->header()->setSectionResizeMode( VcdTrackKeysModel::KeyColumn, QHeaderView::ResizeToContents );

    QVBoxLayout* layout = new QVBoxLayout( d->widgetnumkeys );
    layout->addWidget( d->check_overwritekeys );
    layout->addWidget( d->keys_view );

    d->mainTabbed->addTab( d->widgetnumkeys, i18n( "Numeric Keys" ) );

    connect( d->check_overwritekeys, SIGNAL(toggled(bool)), d->keys_view, SLOT(setEnabled(bool)) );
}

void K3b::VcdTrackDialog::setupAudioTab()
{
    // /////////////////////////////////////////////////
    // AUDIO TAB
    // /////////////////////////////////////////////////
    QWidget * w = new QWidget( d->mainTabbed );

    d->mpegver_audio = new QLabel( w );
    d->rate_audio = new QLabel( w );
    d->sampling_frequency_audio = new QLabel( w );
    d->mode_audio = new QLabel( w );
    d->copyright_audio = new QLabel( w );

    d->mpegver_audio->setFrameShape( QLabel::StyledPanel );
    d->rate_audio->setFrameShape( QLabel::StyledPanel );
    d->sampling_frequency_audio->setFrameShape( QLabel::StyledPanel );
    d->mode_audio->setFrameShape( QLabel::StyledPanel );
    d->copyright_audio->setFrameShape( QLabel::StyledPanel );

    d->mpegver_audio->setFrameShadow( QLabel::Sunken );
    d->rate_audio->setFrameShadow( QLabel::Sunken );
    d->sampling_frequency_audio->setFrameShadow( QLabel::Sunken );
    d->mode_audio->setFrameShadow( QLabel::Sunken );
    d->copyright_audio->setFrameShadow( QLabel::Sunken );

    QFormLayout* layout = new QFormLayout( w );
    layout->setAlignment( Qt::AlignTop );
    layout->setFieldGrowthPolicy( QFormLayout::AllNonFixedFieldsGrow );
    layout->addRow( new QLabel( i18n( "Type:" ), w ),               d->mpegver_audio );
    layout->addRow( new QLabel( i18n( "Rate:" ), w ),               d->rate_audio );
    layout->addRow( new QLabel( i18n( "Sampling frequency:" ), w ), d->sampling_frequency_audio );
    layout->addRow( new QLabel( i18n( "Mode:" ), w ),               d->mode_audio );
    layout->addRow( new QLabel( i18n( "Copyright:" ), w ),          d->copyright_audio );

    d->mainTabbed->addTab( w, i18n( "Audio" ) );

}

void K3b::VcdTrackDialog::setupVideoTab()
{
    // /////////////////////////////////////////////////
    // VIDEO TAB
    // /////////////////////////////////////////////////
    QWidget * w = new QWidget( d->mainTabbed );

    d->mpegver_video = new QLabel( w );
    d->rate_video = new QLabel( w );
    d->chromaformat_video = new QLabel( w );
    d->format_video = new QLabel( w );
    d->resolution_video = new QLabel( w );
    d->highresolution_video = new QLabel( w );

    d->mpegver_video->setFrameShape( QLabel::StyledPanel );
    d->rate_video->setFrameShape( QLabel::StyledPanel );
    d->chromaformat_video->setFrameShape( QLabel::StyledPanel );
    d->format_video->setFrameShape( QLabel::StyledPanel );
    d->resolution_video->setFrameShape( QLabel::StyledPanel );
    d->highresolution_video->setFrameShape( QLabel::StyledPanel );

    d->mpegver_video->setFrameShadow( QLabel::Sunken );
    d->rate_video->setFrameShadow( QLabel::Sunken );
    d->chromaformat_video->setFrameShadow( QLabel::Sunken );
    d->format_video->setFrameShadow( QLabel::Sunken );
    d->resolution_video->setFrameShadow( QLabel::Sunken );
    d->highresolution_video->setFrameShadow( QLabel::Sunken );

    QFormLayout* layout = new QFormLayout( w );
    layout->setAlignment( Qt::AlignTop );
    layout->setFieldGrowthPolicy( QFormLayout::AllNonFixedFieldsGrow );
    layout->addRow( new QLabel( i18n( "Type:" ), w ),            d->mpegver_video );
    layout->addRow( new QLabel( i18n( "Rate:" ), w ),            d->rate_video );
    layout->addRow( new QLabel( i18n( "Chroma format:" ), w ),   d->chromaformat_video );
    layout->addRow( new QLabel( i18n( "Video format:" ), w ),    d->format_video );
    layout->addRow( new QLabel( i18n( "Resolution:" ), w ),      d->resolution_video );
    layout->addRow( new QLabel( i18n( "High resolution:" ), w ), d->highresolution_video );

    d->mainTabbed->addTab( w, i18n( "Video" ) );
}

void K3b::VcdTrackDialog::slotPlayTimeChanged( int value )
{
    if ( value == 0 ) {
        d->labelWait->setEnabled( false );
        d->spin_waittime->setEnabled( false );
        d->labelAfterTimeout->setEnabled( false );
        d->comboAfterTimeout->setEnabled( false );
    } else {
        d->labelWait->setEnabled( true );
        d->spin_waittime->setEnabled( true );
        if ( d->spin_waittime->value() > -1 ) {
            d->labelAfterTimeout->setEnabled( true );
            d->comboAfterTimeout->setEnabled( true );
        }
    }
}

void K3b::VcdTrackDialog::slotWaitTimeChanged( int value )
{
    if ( value < 0 || !d->labelWait->isEnabled() ) {
        d->labelAfterTimeout->setEnabled( false );
        d->comboAfterTimeout->setEnabled( false );
    } else {
        d->labelAfterTimeout->setEnabled( true );
        d->comboAfterTimeout->setEnabled( true );
    }
}

void K3b::VcdTrackDialog::slotPbcToggled( bool checked )
{
    d->groupPlay->setEnabled( checked );
    d->groupPbc->setEnabled( checked );
    d->check_usekeys->setEnabled( checked );
    slotUseKeysToggled( checked && d->check_usekeys->isChecked() );
    d->check_reactivity->setEnabled( checked );
    if ( checked )
        slotWaitTimeChanged( d->spin_waittime->value() );
}

void K3b::VcdTrackDialog::slotUseKeysToggled( bool checked )
{
    d->mainTabbed->setTabEnabled( d->mainTabbed->indexOf( d->widgetnumkeys ), checked );
}


