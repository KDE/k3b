#ifndef K3BAUDIOMODULE
#define K3BAUDIOMODULE


#include <qobject.h>

#include <kurl.h>

class K3bAudioTrack;


/**
 * Abstract streaming class for all the audio input.
 * Has to output data in the following format:
 * MSBLeft LSBLeft MSBRight LSBRight (big endian byte order)
 **/
class K3bAudioModule : public QObject
{
  Q_OBJECT

 public:
  K3bAudioModule( K3bAudioTrack* track );
  virtual ~K3bAudioModule();

  K3bAudioTrack* audioTrack() const { return m_track; }

  /**
   * The consumer is the object that will handle the output
   * Since we need async streaming the AudioModule will
   * produce some output and then wait for the goOnSignal to
   * be emitted by the consumer. When it receives the signal
   * it will produce the next portion of output
   * if consumer is set to null output will be streamed without
   * waiting for the signal (used when writing to a file)
   */
  virtual void setConsumer( QObject* c = 0, const char* goOnSignal = 0 );
  virtual void start() = 0;

 public slots:
  virtual void cancel() = 0;

 signals:
  void output( const unsigned char* data, int len );
  void percent( int );
  void canceled();
  void finished( bool );

 protected slots:
  virtual void slotConsumerReady() = 0;

 protected:
  QObject* m_consumer;

 private:
  K3bAudioTrack* m_track;
};


#endif
