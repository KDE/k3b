/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

// Qt Includes
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmultilineedit.h>
#include <qpixmap.h>
#include <qradiobutton.h>
#include <qtable.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

// Kde Includes
#include <kiconloader.h>
#include <kio/global.h>
#include <klocale.h>
#include <kmimetype.h>
#include <knuminput.h>
#include <kurl.h>

// K3b Includes
#include "k3bvcdtrackdialog.h"
#include "k3bvcdtrack.h"
#include "../kcutlabel.h"
#include "../device/k3bmsf.h"
#include "../tools/k3bglobals.h"
#include "../tools/k3blistview.h"
#include <tools/k3bcutcombobox.h>

K3bVcdTrackDialog::K3bVcdTrackDialog( K3bVcdDoc* _doc, QPtrList<K3bVcdTrack>& tracks, QPtrList<K3bVcdTrack>& selectedTracks, QWidget* parent, const char* name )
        : KDialogBase( KDialogBase::Plain, i18n( "Video Track Properties" ), KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Apply,
                       KDialogBase::Ok, parent, name )
{
    prepareGui();

    setupPbcTab();
    setupPbcKeyTab();
    setupVideoTab();
    setupAudioTab();

    m_tracks = tracks;
    m_selectedTracks = selectedTracks;
    m_vcdDoc = _doc;

    if ( !m_selectedTracks.isEmpty() ) {

        K3bVcdTrack* selectedTrack = m_selectedTracks.first();

        m_displayFileName->setText( selectedTrack->fileName() );
        m_displayLength->setText( selectedTrack->mpegDuration() );
        m_displaySize->setText( KIO::convertSize( selectedTrack->size() ) );

        m_labelMimeType->setPixmap( KMimeType::pixmapForURL( KURL( selectedTrack->absPath() ), 0, KIcon::Desktop, 48 ) );

        fillGui();
    }
}

K3bVcdTrackDialog::~K3bVcdTrackDialog()
{}


void K3bVcdTrackDialog::slotOk()
{
    slotApply();
    done( 0 );
}

void K3bVcdTrackDialog::setPbcTrack( K3bVcdTrack* selected, K3bCutComboBox* box, int which )
{
    // TODO: Unset Userdefined on default settings
    kdDebug() << QString( "K3bVcdTrackDialog::setPbcTrack: currentItem = %1, count = %2" ).arg( box->currentItem() ).arg( m_tracks.count() ) << endl;

    int count = m_tracks.count();

    if ( selected->getPbcTrack( which ) == m_tracks.at( box->currentItem() ) ) {
        if ( selected->getNonPbcTrack( which ) == ( int ) ( box->currentItem() - count ) ) {
            kdDebug() << "K3bVcdTrackDialog::setPbcTrack: not changed, return" << endl;
            return ;
        }
    }

    if ( selected->getPbcTrack( which ) )
        selected->getPbcTrack( which ) ->delFromRevRefList( selected );

    if ( box->currentItem() > count - 1 ) {
        selected->setPbcTrack( which );
        selected->setPbcNonTrack( which, box->currentItem() - count );
    } else {
        selected->setPbcTrack( which, m_tracks.at( box->currentItem() ) );
        m_tracks.at( box->currentItem() ) ->addToRevRefList( selected );
    }

    selected->setUserDefined( which, true );
}

void K3bVcdTrackDialog::slotApply()
{
    K3bVcdTrack* selectedTrack = m_selectedTracks.first();

    setPbcTrack( selectedTrack, m_pbc_previous, K3bVcdTrack::PREVIOUS );
    setPbcTrack( selectedTrack, m_pbc_next, K3bVcdTrack::NEXT );
    setPbcTrack( selectedTrack, m_pbc_return, K3bVcdTrack::RETURN );
    setPbcTrack( selectedTrack, m_pbc_default, K3bVcdTrack::DEFAULT );
    setPbcTrack( selectedTrack, m_comboAfterTimeout, K3bVcdTrack::AFTERTIMEOUT );

    selectedTrack->setPlayTime( m_spin_times->value() );
    selectedTrack->setWaitTime( m_spin_waittime->value() );

}

void K3bVcdTrackDialog::fillGui()
{
    QString tmp;
    K3bVcdTrack* selectedTrack = m_selectedTracks.first();

    if ( selectedTrack->mpegVideoVersion() == 1 ) {
        if ( selectedTrack->hasAudio() )
            m_mpegver_video->setText( i18n( "Mpeg 1 System File [Video/Audio]" ) );
        else
            m_mpegver_video->setText( i18n( "Mpeg 1 System File [Video]" ) );
    } else {
        if ( selectedTrack->hasAudio() )
            m_mpegver_video->setText( i18n( "Mpeg Program Stream File [Video/Audio]" ) );
        else
            m_mpegver_video->setText( i18n( "Mpeg Program Stream File [Video]" ) );
    }

    m_rate_video->setText( i18n( "%1 Mbps" ).arg( selectedTrack->mpegMbps() ) );

    m_duration_video->setText( selectedTrack->mpegDuration() );

    switch ( selectedTrack->MpegAspectRatio() ) {
        case 0:
            m_rate_video->setText( i18n( "Invalid aspect ratio (forbidden)" ) );
            break;
        case 1:
            m_rate_video->setText( i18n( "Aspect ratio 1/1 (VGA)" ) );
            break;
        case 2:
            m_rate_video->setText( i18n( "Aspect ratio 4/3 (TV)" ) );
            break;
        case 3:
            m_rate_video->setText( i18n( "Aspect ratio 16/9 (large TV)" ) );
            break;
        case 4:
            m_rate_video->setText( i18n( "Aspect ratio 2.21/1 (Cinema)" ) );
            break;
        default:
            m_rate_video->setText( i18n( "n/a" ) );
    }

    m_chromaformat_video->setText( i18n( "n/a" ) );

    if ( selectedTrack->MpegSExt() ) {
        if ( selectedTrack->MpegProgressive() )
            tmp = i18n( "Not interlaced" );
        else
            tmp = i18n( "Interlaced" );

        switch ( selectedTrack->MpegChromaFormat() ) {
            case 1 :
                tmp.append( ", 4:2:0" );
                break;
            case 2 :
                tmp.append( ", 4:2:2" );
                break;
            case 3 :
                tmp.append( ", 4:4:4" );
                break;
        }
        m_chromaformat_video->setText( tmp );
    }

    m_format_video->setText( i18n( "n/a" ) );
    if ( selectedTrack->MpegDExt() ) {
        switch ( selectedTrack->MpegFormat() ) {
            case 0 :
                m_format_video->setText( i18n( "Component" ) );
                break;
            case 1 :
                m_format_video->setText( "PAL" );
                break;
            case 2 :
                m_format_video->setText( "NTSC" );
                break;
            case 3 :
                m_format_video->setText( "SECAM" );
                break;
            case 4 :
                m_format_video->setText( "MAC" );
                break;
            case 5 :
                m_format_video->setText( i18n( "Unspecified" ) );
                break;
        }
    }

    m_displaysize_video->setText( selectedTrack->mpegDisplaySize() );
    m_size_video->setText( i18n( "%1 %2 fps %3 Mbps" ).arg( selectedTrack->mpegSize() ).arg( selectedTrack->mpegFps() ).arg( selectedTrack->mpegMbps() ) );


    if ( selectedTrack->hasAudio() ) {
        if ( selectedTrack->MpegAudioType() != 3 )
            m_mpegver_audio->setText( i18n( "Mpeg %1 layer %2" ).arg( selectedTrack->MpegAudioType() ).arg( selectedTrack->MpegAudioLayer() ) );
        else
            m_mpegver_audio->setText( i18n( "Mpeg 2.5 (rare) layer %1" ).arg( selectedTrack->MpegAudioLayer() ) );

        if ( !selectedTrack->MpegAudioKbps().isNull() )
            m_rate_audio->setText( i18n( "%1 kbps %2 Hz" ).arg( selectedTrack->MpegAudioKbps() ).arg( selectedTrack->MpegAudioHz() ) );
        else
            m_rate_audio->setText( i18n( "free bitrate %1 Hz" ).arg( selectedTrack->MpegAudioHz() ) );

        switch ( selectedTrack->MpegAudioMode() ) {
            case 0:
                tmp = i18n( "Stereo" );
                break;
            case 1:
                tmp = i18n( "Joint Stereo" );
                if ( selectedTrack->MpegAudioLayer() == 1 || selectedTrack->MpegAudioLayer() == 2 ) {
                    switch ( selectedTrack->MpegAudioModeExt() ) {
                        case 0:
                            tmp.append( " " + i18n( "(Intensity stereo on bands 4-31/32)" ) );
                            break;
                        case 1:
                            tmp.append( " " + i18n( "(Intensity stereo on bands 8-31/32)" ) );
                            break;
                        case 2:
                            tmp.append( " " + i18n( "(Intensity stereo on bands 12-31/32)" ) );
                            break;
                        case 3:
                            tmp.append( " " + i18n( "(Intensity stereo on bands 16-31/32)" ) );
                            break;
                    }
                } else {
                    //mp3
                    switch ( selectedTrack->MpegAudioModeExt() ) {
                        case 0:
                            tmp.append( " " + i18n( "(Intensity stereo off, M/S stereo off)" ) );
                            break;
                        case 1:
                            tmp.append( " " + i18n( "(Intensity stereo on, M/S stereo off)" ) );
                            break;
                        case 2:
                            tmp.append( " " + i18n( "(Intensity stereo off, M/S stereo on)" ) );
                            break;
                        case 3:
                            tmp.append( " " + i18n( "(Intensity stereo on, M/S stereo on)" ) );
                            break;
                    }
                }
                break;
            case 2:
                tmp = i18n( "Dual Channel" );
                break;
            case 3:
                tmp = i18n( "Mono" );
                break;
        }
        m_mode_audio->setText( tmp );

        switch ( selectedTrack->MpegAudioEmphasis() ) {
            case 0:
                m_emphasis_audio->setText( i18n( "None" ) );
                break;
            case 1:
                m_emphasis_audio->setText( i18n( "50/15 microsecs" ) );
                break;
            case 2:
                m_emphasis_audio->setText( i18n( "Unknown" ) );
                break;
            case 3:
                m_emphasis_audio->setText( i18n( "CCITT J 17" ) );
                break;
        }

        tmp = "";
        if ( selectedTrack->MpegAudioCopyright() )
            tmp.append( "(c) " );
        if ( selectedTrack->MpegAudioOriginal() )
            tmp.append( i18n( "original" ) );
        else
            tmp.append( i18n( "duplicate" ) );

        m_copyright_audio->setText( tmp );
    }

    fillPbcGui();


    QToolTip::add( m_pbc_previous, i18n( "May also look like |<< on the remote control. " ) );
    QToolTip::add( m_pbc_next, i18n( "May also look like >>| on the remote control." ) );
    QToolTip::add( m_pbc_return, i18n( "This key may be mapped to the STOP key." ) );
    QToolTip::add( m_pbc_default, i18n( "This key is usually mapped to the > or PLAY key." ) );
    QToolTip::add( m_comboAfterTimeout, i18n( "Target to be jumped on time-out of <wait>." ) );
		QToolTip::add( m_check_reactivity, i18n( "Delay reactivity of keys." ) );
		QToolTip::add( m_check_pbc, i18n( "Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats." ) );
    QToolTip::add( m_check_usekeys, i18n( "Activate the use of numeric keys." ) );
    QToolTip::add( m_check_overwritekeys, i18n( "Overwrite default numeric keys." ) );
    QToolTip::add( m_list_keys, i18n( "Numeric keys." ) );
    QToolTip::add( m_spin_times, i18n( "Times to repeat the playback of 'play track'." ) );
    QToolTip::add( m_spin_waittime, i18n( "Time in seconds to wait after playback of 'play track'." ) );

		QWhatsThis::add( m_comboAfterTimeout, i18n( "<p>Target to be jumped on time-out of <wait>."
		                                "<p>If omitted (and <wait> is not set to an infinite time) one of the targets is selected at random!" ) );
		QWhatsThis::add( m_check_reactivity, i18n( "<p>When reactivity is set to delayed, it is recommended that the length of the referenced 'play track' is not more than 5 seconds."
		                                "<p>The recommended setting for a play item consisting of one still picture and no audio is to loop once and have a delayed reactivity." ) );
		QWhatsThis::add( m_check_pbc, i18n( "<p>Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats."
                                    "<p>PBC allows control of the playback of play items and the possibility of interaction with the user through the remote control or some other input device available." ) );
    QWhatsThis::add( m_check_usekeys, i18n( "These are actually pseudo keys, representing the numeric keys 0, 1, ..., 9." ) );
    QWhatsThis::add( m_check_overwritekeys, i18n( "<p>If numeric keys enabled, you can overwrite the default settings." ) );
    QWhatsThis::add( m_spin_times, i18n( "<p>Times to repeat the playback of 'play track'."
		                                "<p>The reactivity attribute controls whether the playback of 'play track' is finished, thus delayed, before executing user triggered action or an immediate jump is performed."
																		"<p>After the specified amount of repetitions are completed, the <wait> time begins to count down, unless set to an infinite wait time."
																		"<p>If this element is omitted, a default of `1' is used, i.e. the 'play track' will be displayed once." ) );
    QWhatsThis::add( m_spin_waittime, i18n( "Time in seconds to wait after playback of 'play track' before triggering the <timeout> action (unless the user triggers some action before time ran up)." ) );

}

void K3bVcdTrackDialog::fillPbcGui()
{
    K3bVcdTrack* selectedTrack = m_selectedTracks.first();
    // add tracktitles to combobox
    int iPrevious = -1;
    int iNext = -1;
    int iReturn = -1;
    int iDefault = -1;
    int iAfterTimeOut = -1;

    K3bVcdTrack* track;
    K3bListViewItem* item;
    for ( track = m_tracks.first(); track; track = m_tracks.next() ) {
        QPixmap pm = KMimeType::pixmapForURL( KURL( track->absPath() ), 0, KIcon::Desktop, 16 );
        QString s;
        if ( track == m_selectedTracks.first() ) {
            s = i18n( "ItSelf" );
            item = new K3bListViewItem( m_list_keys, QString( "%1" ).arg( m_pbc_previous->count() ), s );
            item->setEditor( 1, K3bListViewItem::COMBO );
        } else if ( track->isSegment() ) {
            s = i18n( "Segment-%1 - %2" ).arg( QString::number( track->index() + 1 ).rightJustify( 3, '0' ) ).arg( track->title() );
        } else {
            s = i18n( "Sequence-%1 - %2" ).arg( QString::number( track->index() + 1 ).rightJustify( 3, '0' ) ).arg( track->title() );
        }

        m_pbc_previous->insertItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3bVcdTrack::PREVIOUS ) )
            iPrevious = m_pbc_previous->count() - 1;

        m_pbc_next->insertItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3bVcdTrack::NEXT ) )
            iNext = m_pbc_next->count() - 1;

        m_pbc_return->insertItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3bVcdTrack::RETURN ) )
            iReturn = m_pbc_return->count() - 1;

        m_pbc_default->insertItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3bVcdTrack::DEFAULT ) )
            iDefault = m_pbc_default->count() - 1;

        m_comboAfterTimeout->insertItem( pm, s );
        if ( track == selectedTrack->getPbcTrack( K3bVcdTrack::AFTERTIMEOUT ) )
            iAfterTimeOut = m_comboAfterTimeout->count() - 1;

    }

    // add Event Disabled
    QPixmap pmDisabled = SmallIcon( "stop" );
    QString txtDisabled = i18n( "Event Disabled" );
    m_pbc_previous->insertItem( pmDisabled, txtDisabled );
    m_pbc_next->insertItem( pmDisabled, txtDisabled );
    m_pbc_return->insertItem( pmDisabled, txtDisabled );
    m_pbc_default->insertItem( pmDisabled, txtDisabled );
    m_comboAfterTimeout->insertItem( pmDisabled, txtDisabled );

    // add VideoCD End
    QPixmap pmEnd = SmallIcon( "cdrom_unmount" );
    QString txtEnd = i18n( "VideoCD END" );
    m_pbc_previous->insertItem( pmEnd, txtEnd );
    m_pbc_next->insertItem( pmEnd, txtEnd );
    m_pbc_return->insertItem( pmEnd, txtEnd );
    m_pbc_default->insertItem( pmEnd, txtEnd );
    m_comboAfterTimeout->insertItem( pmEnd, txtEnd );

    int count = m_tracks.count();

    if ( iPrevious < 0 )
        m_pbc_previous->setCurrentItem( count + selectedTrack->getNonPbcTrack( K3bVcdTrack::PREVIOUS ) );
    else
        m_pbc_previous->setCurrentItem( iPrevious );

    if ( iNext < 0 )
        m_pbc_next->setCurrentItem( count + selectedTrack->getNonPbcTrack( K3bVcdTrack::NEXT ) );
    else
        m_pbc_next->setCurrentItem( iNext );

    if ( iReturn < 0 )
        m_pbc_return->setCurrentItem( count + selectedTrack->getNonPbcTrack( K3bVcdTrack::RETURN ) );
    else
        m_pbc_return->setCurrentItem( iReturn );

    if ( iDefault < 0 )
        m_pbc_default->setCurrentItem( count + selectedTrack->getNonPbcTrack( K3bVcdTrack::DEFAULT ) );
    else
        m_pbc_default->setCurrentItem( iDefault );

    if ( iAfterTimeOut < 0 )
        m_comboAfterTimeout->setCurrentItem( count + selectedTrack->getNonPbcTrack( K3bVcdTrack::AFTERTIMEOUT ) );
    else
        m_comboAfterTimeout->setCurrentItem( iAfterTimeOut );


    m_spin_times->setValue( selectedTrack->getPlayTime() );
    m_spin_waittime->setValue( selectedTrack->getWaitTime() );

    m_groupPlay->setEnabled( VcdOptions()->PbcEnabled() );
    m_groupPbc->setEnabled( VcdOptions()->PbcEnabled() );

    // not implemented yet
    m_mainTabbed->setTabEnabled( m_widgetnumkeys, VcdOptions()->PbcNumKeys() );
}

void K3bVcdTrackDialog::prepareGui()
{
    QFrame* frame = plainPage();

    QGridLayout* mainLayout = new QGridLayout( frame );
    mainLayout->setSpacing( spacingHint() );
    mainLayout->setMargin( 0 );

    m_mainTabbed = new QTabWidget( frame );

    ///////////////////////////////////////////////////
    // FILE-INFO BOX
    ///////////////////////////////////////////////////
    QGroupBox* groupFileInfo = new QGroupBox( 0, Qt::Vertical, i18n( "File Info" ), frame, "groupFileInfo" );
    groupFileInfo->layout() ->setSpacing( 0 );
    groupFileInfo->layout() ->setMargin( 0 );

    QGridLayout* groupFileInfoLayout = new QGridLayout( groupFileInfo->layout() );
    groupFileInfoLayout->setAlignment( Qt::AlignTop );
    groupFileInfoLayout->setSpacing( spacingHint() );
    groupFileInfoLayout->setMargin( marginHint() );

    m_labelMimeType = new QLabel( groupFileInfo, "m_labelMimeType" );

    m_displayFileName = new KCutLabel( groupFileInfo );
    m_displayFileName->setText( i18n( "Filename" ) );
    m_displayFileName->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );

    QLabel* labelSize = new QLabel( i18n( "Size" ), groupFileInfo, "labelSize" );
    QLabel* labelLength = new QLabel( i18n( "Length" ), groupFileInfo, "labelLength" );

    m_displaySize = new QLabel( groupFileInfo, "m_displaySize" );
    m_displaySize->setText( i18n( "0.0 MB" ) );
    m_displaySize->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

    m_displayLength = new QLabel( groupFileInfo, "m_displayLength" );
    m_displayLength->setText( i18n( "0:0:0" ) );
    m_displayLength->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );

    QFrame* fileInfoLine = new QFrame( groupFileInfo );
    fileInfoLine->setFrameStyle( QFrame::HLine | QFrame::Sunken );

    groupFileInfoLayout->addWidget( m_labelMimeType, 0, 0 );
    groupFileInfoLayout->addMultiCellWidget( m_displayFileName, 0, 1, 1, 1 );
    groupFileInfoLayout->addMultiCellWidget( fileInfoLine, 2, 2, 0, 1 );
    groupFileInfoLayout->addWidget( labelLength, 3, 0 );
    groupFileInfoLayout->addWidget( labelSize, 4, 0 );
    groupFileInfoLayout->addWidget( m_displayLength, 3, 1 );
    groupFileInfoLayout->addWidget( m_displaySize, 4, 1 );

    groupFileInfoLayout->setRowStretch( 5, 1 );
    groupFileInfoLayout->setColStretch( 1, 1 );

    QFont f( m_displayLength->font() );
    f.setBold( true );
    m_displayLength->setFont( f );
    m_displaySize->setFont( f );
    ///////////////////////////////////////////////////

    mainLayout->addWidget( groupFileInfo, 0, 0 );
    mainLayout->addWidget( m_mainTabbed, 0, 1 );

    //  mainLayout->setColStretch( 0, 1 );

}

void K3bVcdTrackDialog::setupPbcTab()
{
    // /////////////////////////////////////////////////
    // Playback Control TAB
    // /////////////////////////////////////////////////
    QWidget* w = new QWidget( m_mainTabbed );

    QGridLayout* grid = new QGridLayout( w );
    grid->setAlignment( Qt::AlignTop );
    grid->setSpacing( spacingHint() );
    grid->setMargin( marginHint() );


    //////////////////////////////////////////////////////////////////////////////////////////
    QGroupBox* groupOptions = new QGroupBox( 3, Qt::Vertical, i18n( "Options" ), w );
    groupOptions->layout() ->setSpacing( spacingHint() );
    groupOptions->layout() ->setMargin( marginHint() );

		m_check_pbc = new QCheckBox( i18n( "Enable Playback Control (PBC)" ), groupOptions, "m_check_pbc" );
		m_check_pbc->hide();

		m_check_usekeys = new QCheckBox( i18n( "Use numeric keys" ), groupOptions, "m_check_usekeys" );
		m_check_usekeys->setEnabled( false );

		m_check_reactivity = new QCheckBox( i18n( "Reactivity delayed to the end of playing track" ), groupOptions, "m_check_reactivity" );
		m_check_reactivity->setEnabled( false );

		//////////////////////////////////////////////////////////////////////////////////////////
		m_groupPlay = new QGroupBox( 0, Qt::Vertical, i18n( "Playing" ), w );
    m_groupPlay->layout() ->setSpacing( spacingHint() );
    m_groupPlay->layout() ->setMargin( marginHint() );

    QGridLayout* groupPlayLayout = new QGridLayout( m_groupPlay->layout() );
    groupPlayLayout->setAlignment( Qt::AlignTop );

    QLabel* labelPlaying = new QLabel( i18n( "Playing track" ) , m_groupPlay, "labelPlaying" );

    m_spin_times = new QSpinBox( m_groupPlay, "m_spin_times" );
    m_spin_times->setValue( 1 );
    m_spin_times->setSuffix( i18n( " time(s)" ) );
    m_spin_times->setSpecialValueText( i18n( "forever" ) );

    //////////////////////////////////////////////////////////////////////////////////////////
    m_labelWait = new QLabel( i18n( "than wait" ), m_groupPlay, "m_labelWait" );
    m_spin_waittime = new QSpinBox( m_groupPlay, "m_spinSeconds" );
    m_spin_waittime->setMinValue( -1 );
    m_spin_waittime->setValue( 0 );
    // m_spin_waittime->setEnabled( false );
    m_spin_waittime->setSuffix( i18n( " seconds" ) );
    m_spin_waittime->setSpecialValueText( i18n( "infinite" ) );

    m_labelAfterTimeout = new QLabel( i18n( "after timeout playing" ), m_groupPlay, "m_labelTimeout" );
    // m_labelAfterTimeout->setEnabled( false );
    m_comboAfterTimeout = new K3bCutComboBox( K3bCutComboBox::SQUEEZE, m_groupPlay, "m_comboAfterTimeout" );
    //    m_comboAfterTimeout->setEnabled( false );

    groupPlayLayout->addWidget( labelPlaying, 1, 0 );
    groupPlayLayout->addWidget( m_spin_times, 1, 1 );
    groupPlayLayout->addWidget( m_labelWait, 1, 2 );
    groupPlayLayout->addWidget( m_spin_waittime, 1, 3 );
    groupPlayLayout->addMultiCellWidget( m_labelAfterTimeout, 2, 2, 1, 3 );
    groupPlayLayout->addMultiCellWidget( m_comboAfterTimeout, 3, 3, 1, 3 );

    //////////////////////////////////////////////////////////////////////////////////////////
    m_groupPbc = new QGroupBox( 0, Qt::Vertical, i18n( "Key Pressed Interaction" ), w );
    m_groupPbc->layout() ->setSpacing( spacingHint() );
    m_groupPbc->layout() ->setMargin( marginHint() );

    QGridLayout* groupPbcLayout = new QGridLayout( m_groupPbc->layout() );
    groupPbcLayout->setAlignment( Qt::AlignTop );

		QLabel* labelPbc_previous = new QLabel( i18n( "Previous:" ), m_groupPbc, "labelPbc_previous" );
    QLabel* labelPbc_next = new QLabel( i18n( "Next:" ), m_groupPbc, "labelPbc_next" );
    QLabel* labelPbc_return = new QLabel( i18n( "Return:" ), m_groupPbc, "labelPbc_return" );
    QLabel* labelPbc_default = new QLabel( i18n( "Default:" ), m_groupPbc, "labelPbc_default" );

    m_pbc_previous = new K3bCutComboBox( K3bCutComboBox::SQUEEZE, m_groupPbc, "m_pbc_previous" );
    m_pbc_next = new K3bCutComboBox( K3bCutComboBox::SQUEEZE, m_groupPbc, "m_pbc_next" );
    m_pbc_return = new K3bCutComboBox( K3bCutComboBox::SQUEEZE, m_groupPbc, "m_pbc_return" );
    m_pbc_default = new K3bCutComboBox( K3bCutComboBox::SQUEEZE, m_groupPbc, "m_pbc_default" );

    groupPbcLayout->addWidget( labelPbc_previous, 1, 0 );
    groupPbcLayout->addMultiCellWidget( m_pbc_previous, 1, 1, 1, 3 );

    groupPbcLayout->addWidget( labelPbc_next, 2, 0 );
    groupPbcLayout->addMultiCellWidget( m_pbc_next, 2, 2, 1, 3 );

    groupPbcLayout->addWidget( labelPbc_return, 3, 0 );
    groupPbcLayout->addMultiCellWidget( m_pbc_return, 3, 3, 1, 3 );

    groupPbcLayout->addWidget( labelPbc_default, 4, 0 );
    groupPbcLayout->addMultiCellWidget( m_pbc_default, 4, 4, 1, 3 );


    grid->addWidget( groupOptions, 0, 0 );
		grid->addWidget( m_groupPlay, 1, 0 );
    grid->addWidget( m_groupPbc, 2, 0 );

    grid->setRowStretch( 9, 1 );

    m_mainTabbed->addTab( w, i18n( "Playback Control" ) );

    connect( m_spin_times, SIGNAL( valueChanged( int ) ), this, SLOT( slotPlayTimeChanged( int ) ) );
    connect( m_spin_waittime, SIGNAL( valueChanged( int ) ), this, SLOT( slotWaitTimeChanged( int ) ) );
}

void K3bVcdTrackDialog::setupPbcKeyTab()
{
    // /////////////////////////////////////////////////
    // Playback Control Numeric Key's TAB
    // /////////////////////////////////////////////////
    m_widgetnumkeys = new QWidget( m_mainTabbed );

    QGridLayout* grid = new QGridLayout( m_widgetnumkeys );
    grid->setAlignment( Qt::AlignTop );
    grid->setSpacing( spacingHint() );
    grid->setMargin( marginHint() );

    QGroupBox* groupKey = new QGroupBox( 3, Qt::Vertical, i18n( "Numeric Keys" ), m_widgetnumkeys );
    groupKey->setEnabled( false );
    groupKey->layout() ->setSpacing( spacingHint() );
    groupKey->layout() ->setMargin( marginHint() );

    m_list_keys = new K3bListView( groupKey, "m_list_keys" );
    m_list_keys->setSorting( 0 );
    m_list_keys->addColumn( i18n( "Key" ) );
    m_list_keys->addColumn( i18n( "Playing" ) );
    m_list_keys->setResizeMode( QListView::LastColumn );

    m_check_overwritekeys = new QCheckBox( i18n( "Overwrite default assignment" ), groupKey, "m_check_overwritekeys" );

    grid->addWidget( groupKey, 1, 0 );

    m_mainTabbed->addTab( m_widgetnumkeys, i18n( "Numeric Keys" ) );
}

void K3bVcdTrackDialog::setupAudioTab()
{
    // /////////////////////////////////////////////////
    // AUDIO TAB
    // /////////////////////////////////////////////////
    QWidget* w = new QWidget( m_mainTabbed );

    QGridLayout* grid = new QGridLayout( w );
    grid->setAlignment( Qt::AlignTop );
    grid->setSpacing( spacingHint() );
    grid->setMargin( marginHint() );

    QLabel* labelMpegVer_Audio = new QLabel( i18n( "Type:" ), w, "labelMpegVer_Audio" );
    QLabel* labelDuration_Audio = new QLabel( i18n( "Estimated duration:" ), w, "labelDuration_Audio" );
    QLabel* labelRate_Audio = new QLabel( i18n( "Rate:" ), w, "labelRate_Audio" );
    QLabel* labelFramesize_Audio = new QLabel( i18n( "Frame size:" ), w, "labelFramesize_Audio" );
    QLabel* labelMode_Audio = new QLabel( i18n( "Mode:" ), w, "labelMode_Audio" );
    QLabel* labelExtMode_Audio = new QLabel( i18n( "Ext. mode:" ), w, "labelExtMode_Audio" );
    QLabel* labelEmphasis_Audio = new QLabel( i18n( "Emphasis:" ), w, "labelEmphasis_Audio" );
    QLabel* labelCopyright_Audio = new QLabel( i18n( "Copyright:" ), w, "labelCopyright_Audio" );

    m_mpegver_audio = new QLabel( w, "m_mpegver_audio" );
    m_duration_audio = new QLabel( w, "m_duration_audio" );
    m_rate_audio = new QLabel( w, "m_rate_audio" );
    m_framesize_audio = new QLabel( w, "m_framesize_audio" );
    m_mode_audio = new QLabel( w, "m_mode_audio" );
    m_extmode_audio = new QLabel( w, "m_extmode_audio" );
    m_emphasis_audio = new QLabel( w, "m_emphasis_audio" );
    m_copyright_audio = new QLabel( w, "m_copyright_audio" );

    m_mpegver_audio->setFrameShape( QLabel::LineEditPanel );
    m_duration_audio->setFrameShape( QLabel::LineEditPanel );
    m_rate_audio->setFrameShape( QLabel::LineEditPanel );
    m_framesize_audio->setFrameShape( QLabel::LineEditPanel );
    m_mode_audio->setFrameShape( QLabel::LineEditPanel );
    m_extmode_audio->setFrameShape( QLabel::LineEditPanel );
    m_emphasis_audio->setFrameShape( QLabel::LineEditPanel );
    m_copyright_audio->setFrameShape( QLabel::LineEditPanel );

    m_mpegver_audio->setFrameShadow( QLabel::Sunken );
    m_duration_audio->setFrameShadow( QLabel::Sunken );
    m_rate_audio->setFrameShadow( QLabel::Sunken );
    m_framesize_audio->setFrameShadow( QLabel::Sunken );
    m_mode_audio->setFrameShadow( QLabel::Sunken );
    m_extmode_audio->setFrameShadow( QLabel::Sunken );
    m_emphasis_audio->setFrameShadow( QLabel::Sunken );
    m_copyright_audio->setFrameShadow( QLabel::Sunken );

    grid->addWidget( labelMpegVer_Audio, 1, 0 );
    grid->addMultiCellWidget( m_mpegver_audio, 1, 1, 1, 4 );

    grid->addWidget( labelDuration_Audio, 2, 0 );
    grid->addMultiCellWidget( m_duration_audio, 2, 2, 1, 4 );

    grid->addWidget( labelRate_Audio, 3, 0 );
    grid->addMultiCellWidget( m_rate_audio, 3, 3, 1, 4 );

    grid->addWidget( labelFramesize_Audio, 4, 0 );
    grid->addMultiCellWidget( m_framesize_audio, 4, 4, 1, 4 );

    grid->addWidget( labelMode_Audio, 5, 0 );
    grid->addMultiCellWidget( m_mode_audio, 5, 5, 1, 4 );

    grid->addWidget( labelExtMode_Audio, 6, 0 );
    grid->addMultiCellWidget( m_extmode_audio, 6, 6, 1, 4 );

    grid->addWidget( labelEmphasis_Audio, 7, 0 );
    grid->addMultiCellWidget( m_emphasis_audio, 7, 7, 1, 4 );

    grid->addWidget( labelCopyright_Audio, 8, 0 );
    grid->addMultiCellWidget( m_copyright_audio, 8, 8, 1, 4 );

    grid->setRowStretch( 9, 4 );

    m_mainTabbed->addTab( w, i18n( "Audio" ) );

}

void K3bVcdTrackDialog::setupVideoTab()
{
    // /////////////////////////////////////////////////
    // VIDEO TAB
    // /////////////////////////////////////////////////
    QWidget* w = new QWidget( m_mainTabbed );

    QGridLayout* grid = new QGridLayout( w );
    grid->setAlignment( Qt::AlignTop );
    grid->setSpacing( spacingHint() );
    grid->setMargin( marginHint() );

    QLabel* labelMpegVer_Video = new QLabel( i18n( "Type:" ), w, "labelMpegVer_Video" );
    QLabel* labelDuration_Video = new QLabel( i18n( "Estimated duration:" ), w, "labelDuration_Video" );
    QLabel* labelRate_Video = new QLabel( i18n( "Rate:" ), w, "labelRate_Video" );
    QLabel* labelChromaFormat_Video = new QLabel( i18n( "Chroma format:" ), w, "labelChromaFormat_Video" );
    QLabel* labelFormat_Video = new QLabel( i18n( "Video format:" ), w, "labelFormat_Video" );
    QLabel* labelSize_Video = new QLabel( i18n( "Size:" ), w, "labelSize_Video" );
    QLabel* labelDisplaySize_Video = new QLabel( i18n( "Display size:" ), w, "labelDisplaySize_Video" );

    m_mpegver_video = new QLabel( w, "m_mpegver_video" );
    m_duration_video = new QLabel( w, "m_duration_video" );
    m_rate_video = new QLabel( w, "m_rate_video" );
    m_chromaformat_video = new QLabel( w, "m_chromaformat_video" );
    m_format_video = new QLabel( w, "m_format_video" );
    m_size_video = new QLabel( w, "m_size_video" );
    m_displaysize_video = new QLabel( w, "m_displaysize_video" );

    m_mpegver_video->setFrameShape( QLabel::LineEditPanel );
    m_duration_video->setFrameShape( QLabel::LineEditPanel );
    m_rate_video->setFrameShape( QLabel::LineEditPanel );
    m_chromaformat_video->setFrameShape( QLabel::LineEditPanel );
    m_format_video->setFrameShape( QLabel::LineEditPanel );
    m_size_video->setFrameShape( QLabel::LineEditPanel );
    m_displaysize_video->setFrameShape( QLabel::LineEditPanel );

    m_mpegver_video->setFrameShadow( QLabel::Sunken );
    m_duration_video->setFrameShadow( QLabel::Sunken );
    m_rate_video->setFrameShadow( QLabel::Sunken );
    m_chromaformat_video->setFrameShadow( QLabel::Sunken );
    m_format_video->setFrameShadow( QLabel::Sunken );
    m_size_video->setFrameShadow( QLabel::Sunken );
    m_displaysize_video->setFrameShadow( QLabel::Sunken );

    grid->addWidget( labelMpegVer_Video, 1, 0 );
    grid->addMultiCellWidget( m_mpegver_video, 1, 1, 1, 4 );

    grid->addWidget( labelDuration_Video, 2, 0 );
    grid->addMultiCellWidget( m_duration_video, 2, 2, 1, 4 );

    grid->addWidget( labelRate_Video, 3, 0 );
    grid->addMultiCellWidget( m_rate_video, 3, 3, 1, 4 );

    grid->addWidget( labelChromaFormat_Video, 4, 0 );
    grid->addMultiCellWidget( m_chromaformat_video, 4, 4, 1, 4 );

    grid->addWidget( labelFormat_Video, 5, 0 );
    grid->addMultiCellWidget( m_format_video, 5, 5, 1, 4 );

    grid->addWidget( labelSize_Video, 6, 0 );
    grid->addMultiCellWidget( m_size_video, 6, 6, 1, 4 );

    grid->addWidget( labelDisplaySize_Video, 7, 0 );
    grid->addMultiCellWidget( m_displaysize_video, 7, 7, 1, 4 );

    grid->setRowStretch( 9, 4 );

    m_mainTabbed->addTab( w, i18n( "Video" ) );
}

void K3bVcdTrackDialog::slotPlayTimeChanged( int value )
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

void K3bVcdTrackDialog::slotWaitTimeChanged( int value )
{
    if ( value < 0 ) {
        m_labelAfterTimeout->setEnabled( false );
        m_comboAfterTimeout->setEnabled( false );
    } else {
        m_labelAfterTimeout->setEnabled( true );
        m_comboAfterTimeout->setEnabled( true );
    }
}

#include "k3bvcdtrackdialog.moc"
