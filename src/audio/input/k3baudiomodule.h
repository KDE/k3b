#ifndef K3BAUDIOMODULE
#define K3BAUDIOMODULE


#include <qobject.h>

class KURL;
class K3bAudioTrack;


/**
 * Abstract class for all the audio input.
 **/
class K3bAudioModule : public QObject
{
  Q_OBJECT

 public:
  K3bAudioModule( K3bAudioTrack* track )
    {
      m_track = track;
    }
  virtual ~K3bAudioModule() {}

  K3bAudioTrack* audioTrack() const { return m_track; }

  /** check if the url contains the correct filetype **/
//  virtual bool valid() const = 0;

  /** has to be async. returns the url the data
      will actually be written (wav files should not be copied!) **/
  virtual KURL writeToWav( const KURL& url ) = 0;
  /** has to be async. Uses signal output() **/
  virtual void getStream() = 0;

 signals:
  void output( char*, int );
  void percent( int );
  void finished();

 private:
  K3bAudioTrack* m_track;
};


#endif
