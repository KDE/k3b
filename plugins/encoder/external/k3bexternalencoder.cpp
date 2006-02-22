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

#include <config.h>

#include "k3bexternalencoder.h"
#include "base_k3bexternalencoderconfigwidget.h"

#include <k3bpluginfactory.h>
#include <k3bprocess.h>
#include <k3bcore.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <klistbox.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

#include <qlayout.h>
#include <qregexp.h>
#include <qtoolbutton.h>
#include <qcheckbox.h>

#include <sys/types.h>
#include <sys/wait.h>


K_EXPORT_COMPONENT_FACTORY( libk3bexternalencoder, K3bPluginFactory<K3bExternalEncoder>( "libk3bexternalencoder" ) )


static const char s_riffHeader[] =
{
  0x52, 0x49, 0x46, 0x46, // 0  "RIFF"
  0x00, 0x00, 0x00, 0x00, // 4  wavSize
  0x57, 0x41, 0x56, 0x45, // 8  "WAVE"
  0x66, 0x6d, 0x74, 0x20, // 12 "fmt "
  0x10, 0x00, 0x00, 0x00, // 16
  0x01, 0x00, 0x02, 0x00, // 20
  0x44, 0xac, 0x00, 0x00, // 24
  0x10, 0xb1, 0x02, 0x00, // 28
  0x04, 0x00, 0x10, 0x00, // 32
  0x64, 0x61, 0x74, 0x61, // 36 "data"
  0x00, 0x00, 0x00, 0x00  // 40 byteCount
};


class K3bExternalEncoder::Command
{
public:
  Command()
    : swapByteOrder(false),
      writeWaveHeader(false) {
  }

  QString name;
  QString extension;
  QString command;

  bool swapByteOrder;
  bool writeWaveHeader;

  int index;  // just used by the config widget
};


static QValueList<K3bExternalEncoder::Command> readCommands()
{
  KConfig* c = k3bcore->config();

  c->setGroup( "K3bExternalEncoderPlugin" );

  QValueList<K3bExternalEncoder::Command> cl;

  QStringList cmds = c->readListEntry( "commands" );
  for( QStringList::iterator it = cmds.begin(); it != cmds.end(); ++it ) {
    QStringList cmdString = c->readListEntry( "command_" + *it );
    K3bExternalEncoder::Command cmd;
    cmd.name = cmdString[0];
    cmd.extension = cmdString[1];
    cmd.command = cmdString[2];
    for( unsigned int i = 3; i < cmdString.count(); ++i ) {
      if( cmdString[i] == "swap" )
	cmd.swapByteOrder = true;
      else if( cmdString[i] == "wave" )
	cmd.writeWaveHeader = true;
    }
    cl.append(cmd);
  }

  // some defaults
  if( cmds.isEmpty() ) {
    // check if the lame encoding plugin has been compiled
#ifndef HAVE_LAME
    K3bExternalEncoder::Command lameCmd;
    lameCmd.name = "Mp3 (Lame)";
    lameCmd.extension = "mp3";
    lameCmd.command = "lame -h --tt %t --ta %a --ty %y --tc %c - %f"; 

    cl.append( lameCmd );
#endif

    if( !KStandardDirs::findExe( "flac" ).isEmpty() ) {
      K3bExternalEncoder::Command flacCmd;
      flacCmd.name = "Flac";
      flacCmd.extension = "flac";
      flacCmd.command = "flac "
	"-V "
	"-o %f "
	"--force-raw-format "
	"--endian=big "
	"--channels=2 "
	"--sample-rate=44100 "
	"--sign=signed "
	"--bps=16 "
	"-T ARTIST=%a "
	"-T TITLE=%t "
	"-";
      
      cl.append( flacCmd );
    }

    if( !KStandardDirs::findExe( "mppenc" ).isEmpty() ) {
      K3bExternalEncoder::Command mppCmd;
      mppCmd.name = "Musepack";
      mppCmd.extension = "mpc";
      mppCmd.command = "mppenc "
	"--standard "
	"--overwrite "
	"--silent "
	"--artist %a "
	"--title %t "
	"--track %n "
	"--album %m "
	"--comment %c "
	"--year %y "
	"- "
	"%f";
      mppCmd.swapByteOrder = true;
      mppCmd.writeWaveHeader = true;
      
      cl.append( mppCmd );
    }
  }

  return cl;
}


static K3bExternalEncoder::Command commandByExtension( const QString& extension )
{
  QValueList<K3bExternalEncoder::Command> cmds( readCommands() );
  for( QValueList<K3bExternalEncoder::Command>::iterator it = cmds.begin(); it != cmds.end(); ++it )
    if( (*it).extension == extension )
      return *it;

  kdDebug() << "(K3bExternalEncoder) could not find command for extension " << extension << endl;

  return K3bExternalEncoder::Command();
}


class K3bExternalEncoder::Private
{
public:
  Private()
    : process(0) {
  }

  K3bProcess* process;
  QString fileName;
  QString extension;
  K3b::Msf length;

  Command cmd;

  bool initialized;

  // the metaData we support
  QString artist;
  QString title;
  QString comment;
  QString trackNumber;
  QString cdArtist;
  QString cdTitle;
  QString cdComment;
  QString year;
  QString genre;
};


K3bExternalEncoder::K3bExternalEncoder( QObject* parent, const char* name )
  : K3bAudioEncoder( parent, name )
{
  d = new Private();
}


K3bExternalEncoder::~K3bExternalEncoder()
{
  delete d->process;
  delete d;
}


void K3bExternalEncoder::setMetaDataInternal( K3bAudioEncoder::MetaDataField f, const QString& value )
{
  switch( f ) {
  case META_TRACK_TITLE:
    d->title = value;
    break;
  case META_TRACK_ARTIST:
    d->artist = value;
    break;
  case META_TRACK_COMMENT:
    d->comment = value;
    break;
  case META_TRACK_NUMBER:
    d->trackNumber = value;
    break;
  case META_ALBUM_TITLE:
    d->cdTitle = value;
    break;
  case META_ALBUM_ARTIST:
    d->cdArtist = value;
    break;
  case META_ALBUM_COMMENT:
    d->cdComment = value;
    break;
  case META_YEAR:
    d->year = value;
    break;
  case META_GENRE:
    d->genre = value;
    break;
  }
}


void K3bExternalEncoder::finishEncoderInternal()
{
  if( d->process ) {
    if( d->process->isRunning() ) {
      ::close( d->process->stdinFd() );

      // this is kind of evil... 
      // but we need to be sure the process exited when this method returnes
      ::waitpid( d->process->pid(), 0, 0 );
    }
  }
}


void K3bExternalEncoder::slotExternalProgramFinished( KProcess* p )
{
  if( !p->normalExit() || p->exitStatus() != 0 )
    kdDebug() << "(K3bExternalEncoder) program exited with error." << endl;
}


bool K3bExternalEncoder::openFile( const QString& ext, const QString& filename, const K3b::Msf& length )
{
  d->fileName = filename;
  d->extension = ext;
  d->initialized = false;
  d->length = length;
  return true;
}


void K3bExternalEncoder::closeFile()
{
  finishEncoderInternal();
}


bool K3bExternalEncoder::initEncoderInternal( const QString& extension )
{
  d->initialized = true;

  // find the correct command
  d->cmd = commandByExtension( extension );

  if( d->cmd.command.isEmpty() ) {
    setLastError( i18n("Invalid command: the command is empty.") );
    return false;
  }

  // setup the process
  delete d->process;
  d->process = new K3bProcess();
  d->process->setSplitStdout(true);
  d->process->setRawStdin(true);

  connect( d->process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotExternalProgramFinished(KProcess*)) );
  connect( d->process, SIGNAL(stderrLine(const QString&)),
	   this, SLOT(slotExternalProgramOutputLine(const QString&)) );
  connect( d->process, SIGNAL(stdoutLine(const QString&)),
	   this, SLOT(slotExternalProgramOutputLine(const QString&)) );


  // create the commandline
  QStringList params = QStringList::split( ' ', d->cmd.command, false );
  for( QStringList::iterator it = params.begin(); it != params.end(); ++it ) {
    (*it).replace( "%f", d->fileName );
    (*it).replace( "%a", d->artist );
    (*it).replace( "%t", d->title );
    (*it).replace( "%c", d->comment );
    (*it).replace( "%y", d->year );
    (*it).replace( "%m", d->cdTitle );
    (*it).replace( "%r", d->cdArtist );
    (*it).replace( "%x", d->cdComment );
    (*it).replace( "%n", d->trackNumber );
    (*it).replace( "%g", d->genre );

    *d->process << *it;
  }


  kdDebug() << "***** external parameters:" << endl;
  const QValueList<QCString>& args = d->process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << flush << endl;

  // set one general error message
  setLastError( i18n("Command failed: %1").arg( s ) );
  
  if( d->process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
    if( d->cmd.writeWaveHeader )
      return writeWaveHeader();
    else
      return true;
  }
  else {
    QString commandName = d->cmd.command.section( QRegExp("\\s+"), 0 );
    if( !KStandardDirs::findExe( commandName ).isEmpty() )
      setLastError( i18n("Could not find program '%1'").arg(commandName) );

    return false;
  }
}


bool K3bExternalEncoder::writeWaveHeader()
{
  kdDebug() << "(K3bExternalEncoder) writing wave header" << endl;

  // write the RIFF thing
  if( ::write( d->process->stdinFd(), s_riffHeader, 4 ) != 4 ) {
    kdDebug() << "(K3bExternalEncoder) failed to write riff header." << endl;
    return false;
  }
  
  // write the wave size
  Q_INT32 dataSize( d->length.audioBytes() );
  Q_INT32 wavSize( dataSize + 44 - 8 );
  char c[4];

  c[0] = (wavSize   >> 0 ) & 0xff;
  c[1] = (wavSize   >> 8 ) & 0xff;
  c[2] = (wavSize   >> 16) & 0xff;
  c[3] = (wavSize   >> 24) & 0xff;
  
  if( ::write( d->process->stdinFd(), c, 4 ) != 4 ) {
    kdDebug() << "(K3bExternalEncoder) failed to write wave size." << endl;
    return false;
  }

  // write static part of the header
  if( ::write( d->process->stdinFd(), s_riffHeader+8, 32 ) != 32 ) {
    kdDebug() << "(K3bExternalEncoder) failed to write wave header." << endl;
    return false;
  }

  c[0] = (dataSize   >> 0 ) & 0xff;
  c[1] = (dataSize   >> 8 ) & 0xff;
  c[2] = (dataSize   >> 16) & 0xff;
  c[3] = (dataSize   >> 24) & 0xff;

  if( ::write( d->process->stdinFd(), c, 4 ) != 4 ) {
    kdDebug() << "(K3bExternalEncoder) failed to write data size." << endl;
    return false;
  }

  return true;
}


long K3bExternalEncoder::encodeInternal( const char* data, Q_ULONG len )
{
  if( !d->initialized )
    if( !initEncoderInternal( d->extension ) )
      return -1;

  if( d->process ) {
    if( d->process->isRunning() ) {

      long written = 0;

      //
      // we swap the bytes to reduce user irritation ;)
      // This is a little confused: We used to swap the byte order
      // in older versions of this encoder since little endian seems
      // to "feel" more natural.
      // So now that we have a swap option we have to invert it to ensure
      // compatibility
      //
      if( !d->cmd.swapByteOrder ) {
	char* buffer = new char[len];
	for( unsigned int i = 0; i < len-1; i+=2 ) {
	  buffer[i] = data[i+1];
	  buffer[i+1] = data[i];
	}

	written = ::write( d->process->stdinFd(), (const void*)buffer, len );
	delete [] buffer;
      }
      else
	written = ::write( d->process->stdinFd(), (const void*)data, len );

      return written;
    }
    else
      return -1;
  }
  else
    return -1;
}


void K3bExternalEncoder::slotExternalProgramOutputLine( const QString& line )
{
  kdDebug() << "(" << d->cmd.name << ") " << line << endl;
}


QStringList K3bExternalEncoder::extensions() const
{
  QStringList el;
  QValueList<K3bExternalEncoder::Command> cmds( readCommands() );
  for( QValueList<K3bExternalEncoder::Command>::iterator it = cmds.begin(); it != cmds.end(); ++it )
    el.append( (*it).extension );

  return el;
}


QString K3bExternalEncoder::fileTypeComment( const QString& ext ) const
{
  return commandByExtension( ext ).name;
}






class K3bExternalEncoderSettingsWidget::Private
{
public:
  QMap<int, K3bExternalEncoder::Command> indexMap;

  int currentCommandIndex;
};


K3bExternalEncoderSettingsWidget::K3bExternalEncoderSettingsWidget( QWidget* parent, const char* name )
  : K3bPluginConfigWidget( parent, name )
{
  d = new Private();
  d->currentCommandIndex = -1;

  w = new base_K3bExternalEncoderConfigWidget( this );

  QHBoxLayout* lay = new QHBoxLayout( this );
  lay->setMargin( 0 );

  lay->addWidget( w );

  w->m_buttonNew->setIconSet( SmallIconSet( "filenew" ) );
  w->m_buttonDelete->setIconSet( SmallIconSet( "editdelete" ) );

  connect( w->m_programList, SIGNAL(highlighted(int)),
	   this, SLOT(slotHighlighted(int)) );
  connect( w->m_buttonNew, SIGNAL(clicked()),
	   this, SLOT(slotNewCommand()) );
  connect( w->m_buttonDelete, SIGNAL(clicked()),
	   this, SLOT(slotDeleteCommand()) );

  connect( w->m_editName, SIGNAL(textChanged(const QString&)),
	   this, SLOT(updateCurrentCommand()) );
  connect( w->m_editExtension, SIGNAL(textChanged(const QString&)),
	   this, SLOT(updateCurrentCommand()) );
  connect( w->m_editCommand, SIGNAL(textChanged(const QString&)),
	   this, SLOT(updateCurrentCommand()) );
  connect( w->m_checkSwapByteOrder, SIGNAL(toggled(bool)),
	   this, SLOT(updateCurrentCommand()) );
  connect( w->m_checkWriteWaveHeader, SIGNAL(toggled(bool)),
	   this, SLOT(updateCurrentCommand()) );
}


K3bExternalEncoderSettingsWidget::~K3bExternalEncoderSettingsWidget()
{
  delete d;
}


void K3bExternalEncoderSettingsWidget::slotDeleteCommand()
{
  if( w->m_programList->currentItem() != -1 ) {
    d->currentCommandIndex = -1; // disable update
    // remove the command and update all indices
    unsigned int i = w->m_programList->currentItem();
    w->m_programList->removeItem( i );
    d->indexMap.remove( i );
    while( i < w->m_programList->count() ) {
      K3bExternalEncoder::Command cmd = d->indexMap[i+1];
      cmd.index--;
      d->indexMap.remove( i+1 );
      d->indexMap.insert( i, cmd );
      i++;
    }

    loadCommand( w->m_programList->currentItem() );
  }
}


void K3bExternalEncoderSettingsWidget::slotNewCommand()
{
  if( checkCurrentCommand() ) {
    K3bExternalEncoder::Command cmd;
    cmd.index = w->m_programList->count();
    d->indexMap.insert( cmd.index, cmd );
    w->m_programList->insertItem( "" );

    w->m_programList->setCurrentItem( cmd.index );
  }
}


void K3bExternalEncoderSettingsWidget::slotHighlighted( int index )
{
  if( checkCurrentCommand() ) {
    loadCommand( index );
  }
  else {
    w->m_programList->blockSignals(true); // prevent recursion
    w->m_programList->setCurrentItem( d->currentCommandIndex );
    w->m_programList->blockSignals(false);
  }
}


void K3bExternalEncoderSettingsWidget::loadCommand( int index )
{
  d->currentCommandIndex = -1; // disable update

  if( index == -1 ) {
    w->m_editName->setText( "" );
    w->m_editExtension->setText( "" );
    w->m_editCommand->setText( "" );
    w->m_checkSwapByteOrder->setChecked( false );
    w->m_checkWriteWaveHeader->setChecked( false );
  }
  else {
    K3bExternalEncoder::Command& cmd = d->indexMap[index];
    w->m_editName->setText( cmd.name );
    w->m_editExtension->setText( cmd.extension );
    w->m_editCommand->setText( cmd.command );
    w->m_checkSwapByteOrder->setChecked( cmd.swapByteOrder );
    w->m_checkWriteWaveHeader->setChecked( cmd.writeWaveHeader );
  }
  
  w->m_editName->setEnabled( index != -1 );
  w->m_editExtension->setEnabled( index != -1 );
  w->m_editCommand->setEnabled( index != -1 );
  w->m_buttonDelete->setEnabled( index != -1 );

  d->currentCommandIndex = index;
}


bool K3bExternalEncoderSettingsWidget::checkCurrentCommand()
{
  if( w->m_programList->count() == 0 || d->currentCommandIndex == -1 )
    return true;

  // we need all entries except the name which will default to the
  // extension if not set

  K3bExternalEncoder::Command& cmd = d->indexMap[d->currentCommandIndex];
  QString name = w->m_editName->text();
  if( name.isEmpty() )
    name = w->m_editExtension->text();

  if( w->m_editExtension->text().isEmpty() ) {
    KMessageBox::error( this, i18n("Please specify an extension.") );
    return false;
  }
  if( w->m_editCommand->text().isEmpty() ) {
    KMessageBox::error( this, i18n("Please specify a command.") );
    return false;
  }
  if( !w->m_editCommand->text().contains( "%f" ) ) {
    KMessageBox::error( this, i18n("The command needs to contain the filename (%f).") );
    return false;
  }

  // we need to make sure the name and the extension are unique
  bool unique = true;
  for( QMap<int, K3bExternalEncoder::Command>::const_iterator it = d->indexMap.begin();
       it != d->indexMap.end(); ++it ) {
    if( ( (*it).name == name || (*it).extension == w->m_editExtension->text() )
	&& (*it).index != cmd.index ) {
      unique = false;
      break;
    }
  }

  if( !unique ) {
    KMessageBox::error( this, i18n("Please specify a unique name and extension.") );
    return false;
  }

  return true;
}


void K3bExternalEncoderSettingsWidget::updateCurrentCommand()
{
  if( d->currentCommandIndex != -1 ) {
    K3bExternalEncoder::Command& cmd = d->indexMap[d->currentCommandIndex];
    QString name = w->m_editName->text();
    if( name.isEmpty() )
      name = w->m_editExtension->text();
    cmd.name = name;
    cmd.extension = w->m_editExtension->text();
    cmd.command = w->m_editCommand->text();
    cmd.swapByteOrder = w->m_checkSwapByteOrder->isChecked();
    cmd.writeWaveHeader = w->m_checkWriteWaveHeader->isChecked();

    w->m_programList->blockSignals(true);
    w->m_programList->changeItem( cmd.name, cmd.index );
    w->m_programList->blockSignals(false);
  }
}


void K3bExternalEncoderSettingsWidget::loadConfig()
{
  w->m_programList->blockSignals(true); // prevent recursion

  d->indexMap.clear();
  w->m_programList->clear();
  d->currentCommandIndex = -1;

  QValueList<K3bExternalEncoder::Command> cmds( readCommands() );
  for( QValueList<K3bExternalEncoder::Command>::iterator it = cmds.begin();
       it != cmds.end(); ++it ) {
    K3bExternalEncoder::Command& cmd = *it;

    cmd.index = w->m_programList->count();
    d->indexMap.insert( cmd.index, cmd );
    w->m_programList->insertItem( cmd.name );
  }

  w->m_programList->blockSignals(false);

  if( !d->indexMap.isEmpty() )
    w->m_programList->setCurrentItem( 0 );
  else
    loadCommand( -1 );
}


void K3bExternalEncoderSettingsWidget::saveConfig()
{
  checkCurrentCommand();

  KConfig* c = k3bcore->config();
  c->deleteGroup( "K3bExternalEncoderPlugin", true );
  c->setGroup( "K3bExternalEncoderPlugin" );

  QStringList cmdNames;
  for( QMapIterator<int, K3bExternalEncoder::Command> it = d->indexMap.begin();
       it != d->indexMap.end(); ++it ) {
    QStringList cmd;
    cmd << it.data().name << it.data().extension << it.data().command;
    if( it.data().swapByteOrder )
      cmd << "swap";
    if( it.data().writeWaveHeader )
      cmd << "wave";
    c->writeEntry( "command_" + it.data().name, cmd );
    cmdNames << it.data().name;
  }
  c->writeEntry( "commands", cmdNames );
}


K3bPluginConfigWidget* K3bExternalEncoder::createConfigWidget( QWidget* parent, 
							       const char* name ) const
{
  return new K3bExternalEncoderSettingsWidget( parent, name );
}


#include "k3bexternalencoder.moc"
