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

#include "k3bstdguiitems.h"

#include <qcheckbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qcombobox.h>
#include <qframe.h>
#include <qpalette.h>

#include <klocale.h>


QCheckBox* K3bStdGuiItems::simulateCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Simulate"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked K3b will perform all writing steps with the "
			   "laser turned off."
			   "<p>This is useful, for example, to test a higher writing speed "
			   "or if your system is able to write on-the-fly.") );
  QToolTip::add( c, i18n("Only simulate the writing process") );
  return c;
}

QCheckBox* K3bStdGuiItems::daoCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Disk at once"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b will write the CD in 'disk at once' mode as "
			   "compared to 'track at once' (TAO)."
			   "<p>It is always recommended to use DAO where possible."
			   "<p><b>Caution:</b> Track pregaps with a length other than 2 seconds are only supported "
			   "in DAO mode.") );
  QToolTip::add( c, i18n("Write in disk at once mode") );
  return c;
}

QCheckBox* K3bStdGuiItems::burnproofCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Burnfree"), parent, name );
  QToolTip::add( c, i18n("Enable Burnfree (or Just Link) to avoid buffer underruns") );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b enables <em>Burnfree</em> "
			   "(or <em>Just Link</em>). This is "
			   "a feature of the CD writer which avoids buffer underruns."
			   "<p>Without <em>burnfree</em> if the writer would not get any more "
			   "data a buffer underrun would occure since the writer needs "
			   "a constant stream of data to write the cd."
			   "<p>With <em>burnfree</em> the writer can <em>mark</em> the current "
			   "position of the laser and get back to it when the buffer is filled again."
			   "But since this means having little data gaps on the cd <b>it is "
			   "highly recommended to always choose an appropriate writing "
			   "speed to prevent the usage of burnfree, especially for audio cds</b> "
			   "(in the worst case one would hear the gap)."
			   "<p><em>Burnfree</em> was formaly known as <em>Burnproof</em> but since "
			   "it has become part of the MMC standard it was renamed.") );
  return c;
}

QCheckBox* K3bStdGuiItems::onlyCreateImagesCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Only create image"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b will only create an "
			   "image and not do any actual writing."
			   "<p>The image can later be written to a CD with most current CD writing "
			   "programs (including K3b of course).") );
  QToolTip::add( c, i18n("Only create an image") );
  return c;
}

QCheckBox* K3bStdGuiItems::removeImagesCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Remove image"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b will remove any created images after the "
			   "writing has finished."
			   "<p>Uncheck this if you want to keep the images.") );
  QToolTip::add( c, i18n("Remove images from harddisk when finished") );
  return c;
}

QCheckBox* K3bStdGuiItems::onTheFlyCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("On the fly"), parent, name );
  QWhatsThis::add( c, i18n("<p>If this option is checked, K3b will not create an image first but write "
			   "the files directly to the CD."
			   "<p><b>Caution:</b> Although this should work on most systems, make sure "
			   "the data is sent to the writer fast enough.")
		   + i18n("<p>It is recommended to try a simulation first.") );
  QToolTip::add( c, i18n("Write files directly to CD without creating an image") );
  return c;
}

QCheckBox* K3bStdGuiItems::cdTextCheckbox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Write CD-TEXT"), parent, name );
  QToolTip::add( c, i18n("Create CD-TEXT entries") );
  QWhatsThis::add( c, i18n("<p>If this option is checked K3b uses some otherwise unused space on the audio "
			   "CD to store additional information, like the artist or the CD title."
			   "<p>CD-TEXT is an extension to the audio CD standard introduced by Sony."
			   "<p>CD-TEXT will only be usable on CD players that support this extension "
			   "(mostly car CD players)."
			   "<p>Since a CD-TEXT enhanced CD will work in any CD player it is never a bad "
			   "idea to enable this (if you specified the data).") );
  return c;
}


QComboBox* K3bStdGuiItems::paranoiaModeComboBox( QWidget* parent, const char* name )
{
  QComboBox* c = new QComboBox( parent, name );
  c->insertItem( "0" );
  c->insertItem( "1" );
  c->insertItem( "2" );
  c->insertItem( "3" );
  c->setCurrentItem( 3 );
  QToolTip::add( c, i18n("Set the paranoia level for reading audio cds") );
  QWhatsThis::add( c, i18n("<p>Sets the correction mode for digital audio extraction."
			   "<ul><li>0: No checking, data is copied directly from the drive. "
			   "<li>1: Perform overlapped reading to avoid jitter.</li>"
			   "<li>2: Like 1 but with additional checks of the read audio data.</li>"
			   "<li>3: Like 2 but with additional scratch detection and repair.</li></ul>"
			   "<p><b>The extraction speed reduces from 0 to 3.</b>") );
  return c;
}


QComboBox* K3bStdGuiItems::dataModeComboboxBox( QWidget* parent, const char* name )
{
  QComboBox* c = new QComboBox( parent, name );
  c->insertItem( "auto" );
  c->insertItem( "mode 1" );
  c->insertItem( "mode 2" );
  c->setCurrentItem( 0 );
  QToolTip::add( c, i18n("Set the data sector type") );
  QWhatsThis::add( c, i18n("<p>Data stored on a CD-ROM is divided into sectors which are equivalent to the audio "
			   "frames on an audio cd. Normal audio sectors contain of 2352 bytes of audio data. "
			   "Due to the additional header data and error correction codes needed for the data "
			   "on a CD-ROM only 2048 bytes can be stored in one sector."
			   "<p>These sectors can be stored in two different modes:"
			   "<ul>"
			   "<li><b>mode 1</b><br>"
			   "Mode 1 sectors were defined in 1985 as part of the <em>Yellow Book</em> "
			   "specification.</li>"
			   "<li><b>mode 2</b><br>"
			   "</li>"
			   "</ul>") );
  return c;
}


QCheckBox* K3bStdGuiItems::startMultisessionCheckBox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Start multisession CD"), parent, name );
  QToolTip::add( c, i18n("Do not close the disk to append additional sessions later") );
  QWhatsThis::add( c, i18n("<p>If this option is checked K3b will not close the cd and write "
			   "a temporary table of contents.</p>"
			   "<p>This allows further sessions to be appended to the CD.</p>") );
  return c;
}


QCheckBox* K3bStdGuiItems::normalizeCheckBox( QWidget* parent, const char* name )
{
  QCheckBox* c = new QCheckBox( i18n("Normalize volume levels"), parent, name );
  QToolTip::add( c, i18n("Adjust the volume levels of all tracks") );
  QWhatsThis::add( c, i18n("<p>If this option is checked K3b will adjust the volume of all tracks "
			   "to a standard level. This is useful for things like creating mixes, "
			   "where different recording levels on different albums can cause the volume "
			   "to vary greatly from song to song."
			   "<p><b>Be aware that K3b currently does not support normalizing when writing "
			   "on the fly!</b>") );
  return c;
}


QFrame* K3bStdGuiItems::purpleFrame( QWidget* parent, const char* name )
{
  QFrame* frame = new QFrame( parent, name );

  // we change the complette palette
  // the colors were created by the QT Designer
  // most of them will never be used...

  QPalette pal;
  QColorGroup cg;
  cg.setColor( QColorGroup::Foreground, Qt::black );
  cg.setColor( QColorGroup::Button, QColor( 205, 210, 255) );
  cg.setColor( QColorGroup::Light, Qt::white );
  cg.setColor( QColorGroup::Midlight, QColor( 230, 232, 255) );
  cg.setColor( QColorGroup::Dark, QColor( 102, 105, 127) );
  cg.setColor( QColorGroup::Mid, QColor( 137, 140, 170) );
  cg.setColor( QColorGroup::Text, Qt::black );
  cg.setColor( QColorGroup::BrightText, Qt::white );
  cg.setColor( QColorGroup::ButtonText, Qt::black );
  cg.setColor( QColorGroup::Base, Qt::white );
  cg.setColor( QColorGroup::Background, QColor( 205, 210, 255) );
  cg.setColor( QColorGroup::Shadow, Qt::black );
  cg.setColor( QColorGroup::Highlight, QColor( 0, 0, 128) );
  cg.setColor( QColorGroup::HighlightedText, Qt::white );
  cg.setColor( QColorGroup::Link, Qt::black );
  cg.setColor( QColorGroup::LinkVisited, Qt::black );
  pal.setActive( cg );
  cg.setColor( QColorGroup::Foreground, Qt::black );
  cg.setColor( QColorGroup::Button, QColor( 205, 210, 255) );
  cg.setColor( QColorGroup::Light, Qt::white );
  cg.setColor( QColorGroup::Midlight, QColor( 243, 244, 255) );
  cg.setColor( QColorGroup::Dark, QColor( 102, 105, 127) );
  cg.setColor( QColorGroup::Mid, QColor( 137, 140, 170) );
  cg.setColor( QColorGroup::Text, Qt::black );
  cg.setColor( QColorGroup::BrightText, Qt::white );
  cg.setColor( QColorGroup::ButtonText, Qt::black );
  cg.setColor( QColorGroup::Base, Qt::white );
  cg.setColor( QColorGroup::Background, QColor( 205, 210, 255) );
  cg.setColor( QColorGroup::Shadow, Qt::black );
  cg.setColor( QColorGroup::Highlight, QColor( 0, 0, 128) );
  cg.setColor( QColorGroup::HighlightedText, Qt::white );
  cg.setColor( QColorGroup::Link, QColor( 0, 0, 192) );
  cg.setColor( QColorGroup::LinkVisited, QColor( 128, 0, 128) );
  pal.setInactive( cg );
  cg.setColor( QColorGroup::Foreground, QColor( 128, 128, 128) );
  cg.setColor( QColorGroup::Button, QColor( 205, 210, 255) );
  cg.setColor( QColorGroup::Light, Qt::white );
  cg.setColor( QColorGroup::Midlight, QColor( 243, 244, 255) );
  cg.setColor( QColorGroup::Dark, QColor( 102, 105, 127) );
  cg.setColor( QColorGroup::Mid, QColor( 137, 140, 170) );
  cg.setColor( QColorGroup::Text, QColor( 128, 128, 128) );
  cg.setColor( QColorGroup::BrightText, Qt::white );
  cg.setColor( QColorGroup::ButtonText, QColor( 128, 128, 128) );
  cg.setColor( QColorGroup::Base, Qt::white );
  cg.setColor( QColorGroup::Background, QColor( 205, 210, 255) );
  cg.setColor( QColorGroup::Shadow, Qt::black );
  cg.setColor( QColorGroup::Highlight, QColor( 0, 0, 128) );
  cg.setColor( QColorGroup::HighlightedText, Qt::white );
  cg.setColor( QColorGroup::Link, QColor( 0, 0, 192) );
  cg.setColor( QColorGroup::LinkVisited, QColor( 128, 0, 128) );
  pal.setDisabled( cg );

  frame->setPalette( pal );
  frame->setFrameShape( QFrame::StyledPanel );
  frame->setFrameShadow( QFrame::Sunken );
  frame->setLineWidth( 2 );
  frame->setMargin( 2 );

  return frame;
}


QFrame* K3bStdGuiItems::horizontalLine( QWidget* parent, const char* name )
{
  QFrame* line = new QFrame( parent, name );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  return line;
}

QFrame* K3bStdGuiItems::verticalLine( QWidget* parent, const char* name )
{
  QFrame* line = new QFrame( parent, name );
  line->setFrameStyle( QFrame::VLine | QFrame::Sunken );
  return line;
}
