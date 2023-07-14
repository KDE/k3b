/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bsoxencoder.h"
#include "k3bsoxencoderdefaults.h"

#include <config-k3b.h>

#include "k3bprocess.h"
#include "k3bcore.h"
#include "k3bexternalbinmanager.h"
#include "k3bplugin_i18n.h"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include <sys/types.h>


K_PLUGIN_CLASS_WITH_JSON(K3bSoxEncoder , "k3bsoxencoder.json")


namespace {

// the sox external program
class SoxProgram : public K3b::ExternalProgram
{
public:
    SoxProgram()
        : K3b::ExternalProgram( "sox" ) {
    }

    bool scan( const QString& p ) override {
        if( p.isEmpty() )
            return false;

        QString path = p;
        QFileInfo fi( path );
        if( fi.isDir() ) {
            path = buildProgramPath( path, "sox" );
        }

        if( !QFile::exists( path ) )
            return false;

        K3b::ExternalBin* bin = 0;

        // probe version
        KProcess vp;
        vp.setOutputChannelMode( KProcess::MergedChannels );

        vp << path << "--version";
        vp.start();
        if( vp.waitForFinished( -1 ) ) {
            QByteArray out = vp.readAll();
            int pos = out.indexOf( "sox: SoX Version" );
            if ( pos >= 0 ) {
                pos += 17;
            }
            else if ( ( pos = out.indexOf( "sox:      SoX v" ) ) >= 0 ) {
                pos += 15;
            }
            else if ( ( pos = out.indexOf( "sox: SoX v" ) ) >= 0 ) {
                pos += 10;
            }
            else if ( ( pos = out.indexOf( "sox: Version" ) ) >= 0 ) {
                pos += 13;
            }
            int endPos = out.indexOf( '\n', pos );
            if( pos > 0 && endPos > 0 ) {
                bin = new K3b::ExternalBin( *this, path );
                bin->setVersion( K3b::Version( out.mid( pos, endPos-pos ) ) );

                addBin( bin );

                return true;
            }
        }

        return false;
    }
};

} // namespace

class K3bSoxEncoder::Private
{
public:
    K3b::Process* process;
    QString fileName;
};


K3bSoxEncoder::K3bSoxEncoder( QObject* parent, const QVariantList& )
    : K3b::AudioEncoder( parent )
{
    if( k3bcore->externalBinManager()->program( "sox" ) == 0 )
        k3bcore->externalBinManager()->addProgram( new SoxProgram() );

    d = new Private();
    d->process = 0;
}


K3bSoxEncoder::~K3bSoxEncoder()
{
    delete d->process;
    delete d;
}


void K3bSoxEncoder::finishEncoderInternal()
{
    if( d->process && d->process->isRunning() ) {
        d->process->closeWriteChannel();

        // this is kind of evil...
        // but we need to be sure the process exited when this method returns
        d->process->waitForFinished(-1);
    }
}


void K3bSoxEncoder::slotSoxFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    if( (exitStatus != QProcess::NormalExit) || (exitCode != 0) )
        qDebug() << "(K3bSoxEncoder) sox exited with error.";
}


bool K3bSoxEncoder::openFile( const QString& extension, const QString& filename, const K3b::Msf& length, const MetaData& metaData )
{
    d->fileName = filename;
    return initEncoderInternal( extension, length, metaData );
}


bool K3bSoxEncoder::isOpen() const
{
    return d->process && d->process->isRunning();
}


void K3bSoxEncoder::closeFile()
{
    finishEncoderInternal();
}


bool K3bSoxEncoder::initEncoderInternal( const QString& extension, const K3b::Msf& /*length*/, const MetaData& /*metaData*/ )
{
    const K3b::ExternalBin* soxBin = k3bcore->externalBinManager()->binObject( "sox" );
    if( soxBin ) {
        // we want to be thread-safe
        delete d->process;
        d->process = new K3b::Process();
        d->process->setSplitStdout(true);

        connect( d->process, SIGNAL(finished(int,QProcess::ExitStatus)),
                 this, SLOT(slotSoxFinished(int,QProcess::ExitStatus)) );
        connect( d->process, SIGNAL(stdoutLine(QString)),
                 this, SLOT(slotSoxOutputLine(QString)) );

        // input settings
        *d->process << soxBin->path()
                    << "-t" << "raw"    // raw samples
                    << "-r" << "44100"  // samplerate
                    << "-s";            // signed linear
        if ( soxBin->version() >= K3b::Version( 13, 0, 0 ) )
            *d->process << "-2";
        else
            *d->process << "-w";        // 16-bit words
        *d->process << "-c" << "2"      // stereo
                    << "-";             // read from stdin

        // output settings
        *d->process << "-t" << extension;

        KSharedConfig::Ptr c = KSharedConfig::openConfig();
        KConfigGroup grp(c,"K3bSoxEncoderPlugin" );
        if( grp.readEntry( "manual settings", DEFAULT_MANUAL_SETTINGS ) ) {
            *d->process << "-r" << QString::number( grp.readEntry( "samplerate", DEFAULT_SAMPLE_RATE ) )
                        << "-c" << QString::number( grp.readEntry( "channels", DEFAULT_CHANNELS ) );

            int size = grp.readEntry( "data size", DEFAULT_DATA_SIZE );
            *d->process << ( size == 8 ? QString("-b") : ( size == 32 ? QString("-l") : QString("-w") ) );

            QString encoding = grp.readEntry( "data encoding", DEFAULT_DATA_ENCODING );
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

        qDebug() << "***** sox parameters:";
        QString s = d->process->joinedArgs();
        qDebug() << s << Qt::flush;

        return d->process->start( KProcess::MergedChannels );
    }
    else {
        qDebug() << "(K3bSoxEncoder) could not find sox bin.";
        return false;
    }
}


qint64 K3bSoxEncoder::encodeInternal( const char* data, qint64 len )
{
    if( d->process && d->process->isRunning() )
        return d->process->write( data, len );
    else
        return -1;
}


void K3bSoxEncoder::slotSoxOutputLine( const QString& line )
{
    qDebug() << "(sox) " << line;
}


QStringList K3bSoxEncoder::extensions() const
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


QString K3bSoxEncoder::fileTypeComment( const QString& ext ) const
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
        return i18n("Wave (SoX)");
    else if( ext == "wve" )
        return i18n("Psion 8-bit A-law");
    else if( ext == "raw" )
        return i18n("Raw");
    else
        return i18n("Error");
}


long long K3bSoxEncoder::fileSize( const QString&, const K3b::Msf& msf ) const
{
    // for now we make a rough assumption based on the settings
    KSharedConfig::Ptr c = KSharedConfig::openConfig();
    KConfigGroup grp(c, "K3bSoxEncoderPlugin" );
    if( grp.readEntry( "manual settings", DEFAULT_MANUAL_SETTINGS ) ) {
        int sr =  grp.readEntry( "samplerate", DEFAULT_SAMPLE_RATE );
        int ch = grp.readEntry( "channels", DEFAULT_CHANNELS );
        int wsize = grp.readEntry( "data size", DEFAULT_DATA_SIZE );

        return msf.totalFrames()*sr*ch*wsize/75;
    }
    else {
        // fallback to raw
        return msf.audioBytes();
    }
}

#include "k3bsoxencoder.moc"

#include "moc_k3bsoxencoder.cpp"
