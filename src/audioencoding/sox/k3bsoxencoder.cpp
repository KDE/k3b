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

#include "k3bsoxencoder.h"
#include "base_k3bsoxencoderconfigwidget.h"

#include <k3bprocess.h>
#include <k3bcore.h>
#include <k3bexternalbinmanager.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>

#include <qfileinfo.h>
#include <qfile.h>
#include <qvalidator.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qwaitcondition.h>



// the sox external program
class K3bSoxProgram : public K3bExternalProgram
{
 public:
  K3bSoxProgram()
    : K3bExternalProgram( "sox" ) {
  }

  bool scan( const QString& p ) {
    if( p.isEmpty() )
      return false;
    
    QString path = p;
    QFileInfo fi( path );
    if( fi.isDir() ) {
      if( path[path.length()-1] != '/' )
	path.append("/");
      path.append("sox");
    }
    
    if( !QFile::exists( path ) )
      return false;
    
    K3bExternalBin* bin = 0;
    
    // probe version
    KProcess vp;
    K3bProcess::OutputCollector out( &vp );
    
    vp << path << "-h";
    if( vp.start( KProcess::Block, KProcess::AllOutput ) ) {
      int pos = out.output().find( "sox: Version" );
      int endPos = out.output().find( "\n", pos );
      if( pos > 0 && endPos > 0 ) {
	pos += 13;
	bin = new K3bExternalBin( this );
	bin->path = path;
	bin->version = out.output().mid( pos, endPos-pos );

	addBin( bin );

	return true;
      }
      else
	return false;
    }
    else
      return false;
  }
};


class K3bSoxEncoder::Private
{
public:
  Private()
    : process(0) {
  }

  K3bProcess* process;
  QString fileName;

  QWaitCondition exitWaiter;
};


K3bSoxEncoder::K3bSoxEncoder( QObject* parent, const char* name )
  : K3bAudioEncoder( parent, name )
{
  d = new Private();
}


K3bSoxEncoder::~K3bSoxEncoder()
{
  delete d->process;
  delete d;
}


void K3bSoxEncoder::finishEncoderInternal()
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


void K3bSoxEncoder::slotSoxFinished( KProcess* p )
{
  if( !p->normalExit() || p->exitStatus() != 0 )
    kdDebug() << "(K3bSoxEncoder) sox exited with error." << endl;
  d->exitWaiter.wakeAll();
}


bool K3bSoxEncoder::openFile( const QString& ext, const QString& filename )
{
  d->fileName = filename;
  return initEncoderInternal( ext );
}


void K3bSoxEncoder::closeFile()
{
  finishEncoderInternal();
}


bool K3bSoxEncoder::initEncoderInternal( const QString& extension )
{
  const K3bExternalBin* soxBin = k3bcore->externalBinManager()->binObject( "sox" );
  if( soxBin ) {
    delete d->process;
    d->process = new K3bProcess();
    d->process->setSplitStdout(true);

    connect( d->process, SIGNAL(processExited(KProcess*)),
	     this, SLOT(slotSoxFinished(KProcess*)) );
    connect( d->process, SIGNAL(stderrLine(const QString&)),
	     this, SLOT(slotSoxOutputLine(const QString&)) );
    connect( d->process, SIGNAL(stdoutLine(const QString&)),
	     this, SLOT(slotSoxOutputLine(const QString&)) );

    // input settings
    *d->process << soxBin->path 
		<< "-t" << "raw"    // raw samples
		<< "-r" << "44100"  // samplerate
		<< "-s"             // signed linear
		<< "-w"             // 16-bit words
		<< "-c" << "2"      // stereo
		<< "-";             // read from stdin

    // output settings
    *d->process << "-t" << extension;

    KConfig* c = k3bcore->config();
    c->setGroup( "K3bSoxEncoderPlugin" );
    if( c->readBoolEntry( "manual settings", false ) ) {
      *d->process << "-r" << QString::number( c->readNumEntry( "samplerate", 44100 ) )
		  << "-c" << QString::number( c->readNumEntry( "channels", 2 ) );

      int size = c->readNumEntry( "data size", 16 );
      *d->process << ( size == 8 ? QString("-b") : ( size == 32 ? QString("-l") : QString("-w") ) );

      QString encoding = c->readEntry( "data encoding", "signed" );
      if( encoding == "unsigned" )
	*d->process << "-u";
      else if( encoding == "u-law" )
	*d->process << "-U";
      else if( encoding == "A-law" )
	*d->process << "-A";
      else if( encoding == "ADPCM" )
	*d->process << "-a";
      else if( encoding == "IMA_ADPCM" )
	*d->process << "-i";
      else if( encoding == "GSM" )
	*d->process << "-g";
      else if( encoding == "Floating-point" )
	*d->process << "-f";
      else
	*d->process << "-s";
    }

    *d->process << d->fileName;

    kdDebug() << "***** sox parameters:" << endl;
    const QValueList<QCString>& args = d->process->args();
    QString s;
    for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
      s += *it + " ";
    }
    kdDebug() << s << flush << endl;


    return d->process->start( KProcess::NotifyOnExit, KProcess::All );
  }
  else {
    kdDebug() << "(K3bSoxEncoder) could not find sox bin." << endl;
    return false;
  }
}


long K3bSoxEncoder::encodeInternal( const char* data, Q_ULONG len )
{
  if( d->process ) {
    if( d->process->isRunning() )
      return ::write( d->process->stdinFd(), (const void*)data, len );
    else
      return -1;
  }
  else
    return -1;
}


void K3bSoxEncoder::slotSoxOutputLine( const QString& line )
{
  kdDebug() << "(sox) " << line << endl;
}



K3bSoxEncoderFactory::K3bSoxEncoderFactory( QObject* parent, const char* name )
  : K3bAudioEncoderFactory( parent, name )
{
  if( k3bcore->externalBinManager()->program( "sox" ) == 0 )
    k3bcore->externalBinManager()->addProgram( new K3bSoxProgram() );

  s_instance = new KInstance( "k3bsoxencoder" );
}


K3bSoxEncoderFactory::~K3bSoxEncoderFactory()
{
}


QStringList K3bSoxEncoderFactory::extensions() const
{
  static QStringList s_extensions;
  if( s_extensions.isEmpty() ) {
    s_extensions << "au" 
		 << "8svx"
		 << "aiff"
		 << "avr"
		 << "cdr"
		 << "cvs"
		 << "dat"
		 << "gsm"
		 << "hcom"
		 << "maud"
		 << "sf"
		 << "sph"
		 << "smp"
		 << "txw"
		 << "vms"
		 << "voc"
		 << "wav"
		 << "wve"
		 << "raw";
  }

  if( k3bcore->externalBinManager()->foundBin( "sox" ) )
    return s_extensions;
  else
    return QStringList(); // no sox -> no encoding
}


QString K3bSoxEncoderFactory::fileTypeComment( const QString& ext ) const
{
  if( ext == "au" )
    return i18n("Sun AU");
  else if( ext == "8svx" )
    return i18n("Amiga 8SVX");
  else if( ext == "aiff" )
    return i18n("AIFF");
  else if( ext == "avr" )
    return i18n("Audio Visual Research");
  else if( ext == "cdr" )
    return i18n("CD-R");
  else if( ext == "cvs" )
    return i18n("CVS");
  else if( ext == "dat" )
    return i18n("Text Data");
  else if( ext == "gsm" )
    return i18n("GSM Speech");
  else if( ext == "hcom" )
    return i18n("Macintosh HCOM");
  else if( ext == "maud" )
    return i18n("Maud (Amiga)");
  else if( ext == "sf" )
    return i18n("IRCAM");
  else if( ext == "sph" )
    return i18n("SPHERE");
  else if( ext == "smp" )
    return i18n("Turtle Beach SampleVision");
  else if( ext == "txw" )
    return i18n("Yamaha TX-16W");
  else if( ext == "vms" )
    return i18n("VMS");
  else if( ext == "voc" )
    return i18n("Sound Blaster VOC");
  else if( ext == "wav" )
    return i18n("Wave (Sox)");
  else if( ext == "wve" )
    return i18n("Psion 8-bit A-law");
  else if( ext == "raw" )
    return i18n("Raw");
  else
    return i18n("Error");
}


long long K3bSoxEncoderFactory::fileSize( const QString&, const K3b::Msf& msf ) const
{
  // for now we make a rough assumption based on the settings
    KConfig* c = k3bcore->config();
    c->setGroup( "K3bSoxEncoderPlugin" );
    if( c->readBoolEntry( "manual settings", false ) ) {
      int sr =  c->readNumEntry( "samplerate", 44100 );
      int ch = c->readNumEntry( "channels", 2 );
      int wsize = c->readNumEntry( "data size", 16 );

      return msf.totalFrames()*sr*ch*wsize/75;
    }
    else {
      // fallback to raw
      return msf.audioBytes();
    }
}


K3bPlugin* K3bSoxEncoderFactory::createPluginObject( QObject* parent, 
						     const char* name,
						     const QStringList& )
{
  return new K3bSoxEncoder( parent, name );
}


K3bPluginConfigWidget* K3bSoxEncoderFactory::createConfigWidgetObject( QWidget* parent, 
								       const char* name,
								       const QStringList& )
{
  return new K3bSoxEncoderSettingsWidget( parent, name );
}



K3bSoxEncoderSettingsWidget::K3bSoxEncoderSettingsWidget( QWidget* parent, const char* name )
  : K3bPluginConfigWidget( parent, name )
{
  w = new base_K3bSoxEncoderConfigWidget( this );
  w->m_editSamplerate->setValidator( new QIntValidator( w->m_editSamplerate ) );

  QHBoxLayout* lay = new QHBoxLayout( this );
  lay->setMargin( 0 );

  lay->addWidget( w );
}


K3bSoxEncoderSettingsWidget::~K3bSoxEncoderSettingsWidget()
{
}


void K3bSoxEncoderSettingsWidget::loadConfig()
{
  KConfig* c = k3bcore->config();

  c->setGroup( "K3bSoxEncoderPlugin" );

  w->m_checkManual->setChecked( c->readBoolEntry( "manual settings", false ) );

  int channels = c->readNumEntry( "channels", 2 );
  w->m_comboChannels->setCurrentItem( channels == 4 ? 2 : channels-1 );

  w->m_editSamplerate->setText( QString::number( c->readNumEntry( "samplerate", 44100 ) ) );

  QString encoding = c->readEntry( "data encoding", "signed" );
  if( encoding == "unsigned" )
    w->m_comboEncoding->setCurrentItem(1);
  else if( encoding == "u-law" )
    w->m_comboEncoding->setCurrentItem(2);
  else if( encoding == "A-law" )
    w->m_comboEncoding->setCurrentItem(3);
  else if( encoding == "ADPCM" )
    w->m_comboEncoding->setCurrentItem(4);
  else if( encoding == "IMA_ADPCM" )
    w->m_comboEncoding->setCurrentItem(5);
  else if( encoding == "GSM" )
    w->m_comboEncoding->setCurrentItem(6);
  else if( encoding == "Floating-point" )
    w->m_comboEncoding->setCurrentItem(7);
  else
    w->m_comboEncoding->setCurrentItem(0);

  int size = c->readNumEntry( "data size", 16 );
  w->m_comboSize->setCurrentItem( size == 8 ? 0 : ( size == 32 ? 2 : 1 ) );
}


void K3bSoxEncoderSettingsWidget::saveConfig()
{
  KConfig* c = k3bcore->config();

  c->setGroup( "K3bSoxEncoderPlugin" );

  c->writeEntry( "manual settings", w->m_checkManual->isChecked() );

  c->writeEntry( "channels", w->m_comboChannels->currentItem() == 0 
		 ? 1 
		 : ( w->m_comboChannels->currentItem() == 2 
		     ? 4 
		     : 2 ) );

  c->writeEntry( "data size", w->m_comboSize->currentItem() == 0 
		 ? 8
		 : ( w->m_comboSize->currentItem() == 2
		     ? 32
		     : 16 ) );

  c->writeEntry( "samplerate", w->m_editSamplerate->text().toInt() );

  QString enc;
  switch( w->m_comboEncoding->currentItem() ) {
  case 1:
    enc = "unsigned";
    break;
  case 2:
    enc = "u-law";
    break;
  case 3:
    enc = "A-law";
    break;
  case 4:
    enc = "ADPCM";
    break;
  case 5:
    enc = "IMA_ADPCM";
    break;
  case 6:
    enc = "GSM";
    break;
  case 7:
    enc = "Floating-point";
    break;
  default:
    enc = "signed";
    break;
  }
  c->writeEntry( "data encoding", enc );
}


#include "k3bsoxencoder.moc"
