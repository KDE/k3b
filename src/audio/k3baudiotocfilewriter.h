#ifndef K3B_AUDIO_TOCFILE_WRITER_H
#define K3B_AUDIO_TOCFILE_WRITER_H

class K3bAudioDoc;
class K3bAudioTrack;
class QString;
class QTextStream;


class K3bAudioTocfileWriter
{
 public:
  static bool writeAudioTocFile( K3bAudioDoc*, const QString& filename );
  static bool writeAudioToc( K3bAudioDoc*, QTextStream& );

 private:
  static void writeCdTextEntries( K3bAudioTrack* track, QTextStream& t );
  static QString prepareForTocFile( const QString& str );
};

#endif
