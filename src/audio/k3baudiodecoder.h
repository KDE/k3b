#ifndef K3B_AUDIO_DECODER_H
#define K3B_AUDIO_DECODER_H

#include <k3bjob.h>

class K3bAudioDoc;
class K3bAudioTrack;


/**
 * Decodes all tracks of an audio project into one raw audio data stream.
 * In the future it will also take care of the filtering.
 */
class K3bAudioDecoder : public K3bJob
{
  Q_OBJECT

 public:
  K3bAudioDecoder( K3bAudioDoc*, QObject* parent = 0, const char* name = 0 );
  ~K3bAudioDecoder();

 public slots:
  void start();
  void cancel();
  void resume();

 signals:
  void data( const char* data, int len );
  void nextTrack( int );
  // just needed for the dumb module consumer api
  void resumeDecoding();

 private slots:
  void slotModuleOutput( const unsigned char*, int len );
  void slotModuleFinished( bool );
  void slotModulePercent( int );

 private:
  void decodeNextTrack();
  K3bAudioDoc* m_doc;
  K3bAudioTrack* m_currentTrack;
  int m_currentTrackNumber;
  bool m_canceled;
  unsigned long m_decodedDataSize;
  unsigned long m_docSize;
};

#endif
