#ifndef K3BAUDIOMODULE
#define K3BAUDIOMODULE


#include <qobject.h>
#include <qptrlist.h>

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
  K3bAudioModule( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bAudioModule();

  K3bAudioTrack* audioTrack() const { return m_track; }

  void start( K3bAudioTrack* );

  virtual bool canDecode( const KURL& url ) = 0;

  bool allTracksAnalysed();

 public slots:
  virtual void cancel() = 0;

  virtual void resume();
 
  /** 
  * add a track that should be analysed by 
  * this module
  */
  void addTrackToAnalyse( K3bAudioTrack* );

  /**
   * called by ~K3bAudioTrack
   * to stop analysing 
   */
  void removeTrackToAnalyse( K3bAudioTrack* track );

 signals:
  /**
   * after emitting this signal the module has to be resumed
   */
  void output( const unsigned char* data, int len );
  void percent( int );
  void canceled();
  void finished( bool );
  /**
   *  this signal has to be emitted when
   * analysing is finished */
  void trackAnalysed( K3bAudioTrack* );

 protected slots:
  virtual void slotConsumerReady() {}
  virtual void startDecoding() = 0;

  /**
   * retrieve information about the track like the length
   * emit trackAnalysed signal when finished.
   */
  virtual void analyseTrack() = 0;
  virtual void stopAnalysingTrack() = 0;

 private slots:
  void slotAnalysingFinished( K3bAudioTrack* );

 private:
  K3bAudioTrack* m_track;

  QPtrList<K3bAudioTrack> m_tracksToAnalyse;
  K3bAudioTrack* m_currentlyAnalysedTrack;
};


#endif
