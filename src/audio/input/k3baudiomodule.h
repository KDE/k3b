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
  K3bAudioModule( K3bAudioTrack* track )
    {
      m_track = track;
    }
  virtual ~K3bAudioModule() {}

  K3bAudioTrack* audioTrack() const { return m_track; }

  /**
   * can be reimplemented to read data like id3-tags in mp3-files
   * will only be called if added to a project.
   * but not when loading a project.
   */
  virtual void init() {}

  /** check if the url contains the correct filetype **/
//  virtual bool valid() const = 0;

  /** has to be async. returns the url the data
      will actually be written (wav files should not be copied!) **/
  virtual KURL writeToWav( const KURL& url = KURL() ) = 0;
  /** has to be async. Uses signal output() **/
  virtual bool getStream() = 0;

  /**
   * must return the number of actually read bytes
   */
  virtual int readData( char*, int ) = 0;

 public slots:
  virtual void cancel() = 0;

 signals:
  /**
   * when this signal is emmited new output is availible and can be read
   * with readData().
   * Note that the module stops working until readData() is called.
   * @param len The length of the data being available.
   */
  void output( int len );
  void percent( int );
  void finished( bool );

 private:
  K3bAudioTrack* m_track;
};


#endif
