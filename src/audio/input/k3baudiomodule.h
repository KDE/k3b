#ifndef K3BAUDIOMODULE
#define K3BAUDIOMODULE


#include <qobject.h>

#include <kurl.h>

class K3bAudioTrack;


/**
 * Abstract class for all the audio input.
 **/
class K3bAudioModule : public QObject
{
  Q_OBJECT

 public:
  K3bAudioModule( K3bAudioTrack* track );
  virtual ~K3bAudioModule();

  K3bAudioTrack* audioTrack() const { return m_track; }

  /** check if the url contains the correct filetype **/
//  virtual bool valid() const = 0;

  /**
   * start decoding from relative position
   */
  virtual void start( double offset = 0.0 ) = 0;

 public slots:
  virtual void cancel() = 0;
  virtual void pause() = 0;
  virtual void resume() = 0;

 signals:
  void output( const unsigned char* data, int len );
  void percent( int );
  void canceled();
  void finished( bool );

 private:
  K3bAudioTrack* m_track;
};


#endif
