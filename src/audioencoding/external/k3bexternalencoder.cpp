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

#include "k3bexternalencoder.h"
#include "base_k3bexternalencoderconfigwidget.h"

#include <k3bprocess.h>
#include <k3bcore.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <klistbox.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qregexp.h>
#include <qtoolbutton.h>
#include <qwaitcondition.h>



class K3bExternalEncoder::Command
{
public:
  QString name;
  QString extension;
  QString command;

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
    cl.append(cmd);
  }

  // some defaults
  if( cmds.isEmpty() ) {
    K3bExternalEncoder::Command lameCmd, flacCmd;
    lameCmd.name = "Lame";
    lameCmd.extension = "mp3";
    lameCmd.command = "lame -h --tt %t --ta %a --ty %y --tc %c - %f"; 

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

    cl.append( lameCmd );
    cl.append( flacCmd );
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

  Command cmd;

  bool initialized;

  // the metaData we support
  QString artist;
  QString title;
  QString comment;
  QString year;

  QWaitCondition exitWaiter;
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


void K3bExternalEncoder::setMetaDataInternal( const QString& key, const QString& value )
{
  if( key.lower() == "title" )
    d->title = value;
  else if( key.lower() == "artist" )
    d->artist = value;
  else if( key.lower() == "comment" )
    d->comment = value;
  else if( key.lower() == "year" )
    d->year = value;
}


void K3bExternalEncoder::finishEncoderInternal()
{
  if( d->process ) {
    if( d->process->isRunning() ) {
      d->process->closeStdin();

      // this is kind of evil... 
      // but we need to be sure the process exited when this method returnes
      d->exitWaiter.wait();
    }
  }
}


void K3bExternalEncoder::slotExternalProgramFinished( KProcess* p )
{
  if( !p->normalExit() || p->exitStatus() != 0 )
    kdDebug() << "(K3bExternalEncoder) program exited with error." << endl;
  d->exitWaiter.wakeAll();
}


bool K3bExternalEncoder::openFile( const QString& ext, const QString& filename )
{
  d->fileName = filename;
  d->extension = ext;
  d->initialized = false;
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
    kdDebug() << "(K3bExternalEncoder) empty command for extension " << extension << endl;
    return false;
  }

  // setup the process
  delete d->process;
  d->process = new K3bProcess();
  d->process->setSplitStdout(true);
  
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

    *d->process << *it;
  }


  kdDebug() << "***** external parameters:" << endl;
  const QValueList<QCString>& args = d->process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << flush << endl;
  
  return d->process->start( KProcess::NotifyOnExit, KProcess::All );
}


long K3bExternalEncoder::encodeInternal( const char* data, Q_ULONG len )
{
  if( !d->initialized )
    if( !initEncoderInternal( d->extension ) )
      return -1;

  if( d->process ) {
    if( d->process->isRunning() ) {

      // we swap the bytes to reduce user irritation ;)
      char* buffer = new char[len];
      for( unsigned int i = 0; i < len-1; i+=2 ) {
	buffer[i] = data[i+1];
	buffer[i+1] = data[i];
      }

      long written = ::write( d->process->stdinFd(), (const void*)buffer, len );

      delete [] buffer;

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



K3bExternalEncoderFactory::K3bExternalEncoderFactory( QObject* parent, const char* name )
  : K3bAudioEncoderFactory( parent, name )
{
  s_instance = new KInstance( "k3bexternalencoder" );
}


K3bExternalEncoderFactory::~K3bExternalEncoderFactory()
{
}


QStringList K3bExternalEncoderFactory::extensions() const
{
  QStringList el;
  QValueList<K3bExternalEncoder::Command> cmds( readCommands() );
  for( QValueList<K3bExternalEncoder::Command>::iterator it = cmds.begin(); it != cmds.end(); ++it )
    el.append( (*it).extension );

  return el;
}


QString K3bExternalEncoderFactory::fileTypeComment( const QString& ext ) const
{
  return commandByExtension( ext ).name;
}



K3bPlugin* K3bExternalEncoderFactory::createPluginObject( QObject* parent, 
						     const char* name,
						     const QStringList& )
{
  return new K3bExternalEncoder( parent, name );
}


K3bPluginConfigWidget* K3bExternalEncoderFactory::createConfigWidgetObject( QWidget* parent, 
								       const char* name,
								       const QStringList& )
{
  return new K3bExternalEncoderSettingsWidget( parent, name );
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
}


K3bExternalEncoderSettingsWidget::~K3bExternalEncoderSettingsWidget()
{
  delete d;
}


void K3bExternalEncoderSettingsWidget::slotDeleteCommand()
{
  if( w->m_programList->currentItem() != -1 ) {
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
  }
  else {
    K3bExternalEncoder::Command& cmd = d->indexMap[index];
    w->m_editName->setText( cmd.name );
    w->m_editExtension->setText( cmd.extension );
    w->m_editCommand->setText( cmd.command );
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
    c->writeEntry( "command_" + it.data().name, cmd );
    cmdNames << it.data().name;
  }
  c->writeEntry( "commands", cmdNames );
}


#include "k3bexternalencoder.moc"
