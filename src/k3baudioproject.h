
#ifndef K3BAUDIOPROJECT_H
#define K3BAUDIOPROJECT_H

#include "k3bproject.h"

#include <qstring.h>
#include <qdatetime.h>

class K3bAudioTrack;

/**
 * Burn an audio cd.
 * To use it create an instance and add tracks with one of the
 * addTrack methods.
 * Modify the track-settings and project settings like artist and stuff.
 * To burn the cd one should at first call @p prepareTracks() while
 * cdrecord (which is used for burning so far) does not support
 * burning of mp3-files. After that a call of write() starts the burning.
 * One should at least connect to the result() signal which is emmited
 * when the task is completed.
 * A call of result() returns an error code that informs about the result.
 **/
class K3bAudioProject : public K3bProject
{
	Q_OBJECT

public:
	K3bAudioProject( const QString& projectName,
					 const QString& cdrecord,
					 const QString& mpg123,
					 QObject* parent = 0, const char* name = 0 );
	~K3bAudioProject();

	/** Convert mp3 into wav since cdrecord is used **/
	void prepareTracks();
	void write();
	void writeImage( const QString& filename );

	const QTime& audioSize() const;
	int numberOfTracks() const { return m_tracks.count(); }

	K3bAudioTrack* current() const { return m_tracks.current(); }
	K3bAudioTrack* next() { return m_tracks.next(); }
	K3bAudioTrack* prev() { return m_tracks.prev(); }
	K3bAudioTrack* at( uint i ) { return m_tracks.at( i ); }
	K3bAudioTrack* take( uint i ) { return m_tracks.take( i ); }

  /** get the current size of the project */
  int size();

protected:
	void addMp3File( const QString& fileName, uint position );
	void addWavFile( const QString& fileName, uint position );
	
public slots:
	/**
	 * will test the file and add it to the project.
	 * connect to at least result() to know when
	 * the process is finished and check error()
	 * to know about the result.
	 **/
	void addTrack( const QString& filename, uint position = 0 );
	/** adds a track without any testing */
	void addTrack( K3bAudioTrack* track, uint position = 0 );
//	void addTracks( QList<K3bAudioTrack>& tracks );
	void removeTrack( uint position );
	void clear();
	void moveTrack( uint oldPos, uint newPos );

protected slots:
	void startRecording();
	void parseCdrecordOutput( KProcess*, char* output, int len );
	void cdrecordFinished();
	void bufferFiles( );
	void parseMpg123Output( KProcess*, char* output, int len );
	void fileBufferingFinished();
	void mp3FileTestingFinished();

signals:
	void newTrack( K3bAudioTrack* );
	void trackRemoved( uint );
	void nextTrackProcessed();
	void trackPercent( unsigned long precent );
	void trackProcessedSize( unsigned long size );
	void trackProcessedMinutes( const QTime& );
	void processedMinutes( const QTime& );

private:
	bool startBurningAfterBuffering;

	QList<K3bAudioTrack> m_tracks;
	K3bAudioTrack* m_currentBufferedTrack;
	K3bAudioTrack* m_lastAddedTrack;

	/** path to mpg123 executable **/
	QString m_mpg123;

 	QString lastTempFile;
 	QString lastTestedFile;
 	int lastAddedPosition;

 	// settings
 	/** if true the adding of files will take longer */
 	bool testFiles;
};


#endif
