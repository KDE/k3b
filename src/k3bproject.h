

#ifndef K3BPROJECT_H
#define K3BPROJECT_H

// QT-includes
#include <qlist.h>
#include <qstring.h>
#include <qobject.h>

#include "k3bdevicemanager.h"


class QTimer;
class KTempFile;
class K3bDevice;
class KProcess;


enum Error { NOT_STARTED = 1, SUCCESS = 2, IMAGE_FINISHED = 3,
			 CANCELED = 4, FILE_NOT_FOUND = 5,
			 BUFFER_UNDERRUN = 6, WRITE_ERROR = 7,
			 COULD_NOT_OPEN_IMAGE = 8, DEVICE_NOT_FOUND = 9,
			 NO_TRACKS = 10, WORKING = 11, CDRECORD_ERROR = 12,
			 MPG123_ERROR = 13, WRONG_FILE_FORMAT = 14, CORRUPT_MP3 = 15};

enum FileType { MP3 = 1, WAV = 2 };



/**
 * All you need to burn a cd.
 * Normal use: Create an instance beloning to your needs, i.e.
 * K3bAudioProject. Add Tracks in two ways: construct K3bAudioProject::AudioTrack
 * instances on your own and add them with addTrack or add only
 * their url and let the project handle the rest.
 **/
class K3bProject : public QObject
{
	Q_OBJECT

public:
	/**
	 * @param cdrecord The path to the cdrecord program on the local system
	 *                 In further versions this should be obsolet while
	 *                 I plan to write my own burning-code.
	 **/
	K3bProject( const QString& projectName,
							const QString& cdrecord,
							QObject* parent = 0, const char* name = 0 );
	virtual ~K3bProject();

	/**
	 * For writing "on the fly "
	 * Connect to the signals (at least result())
	 * which will be deleted after the writing finished. To get the result
	 * use error().
	 **/
	virtual void write() = 0;

	/**
	 * Writing an image file to cdr. To create an image use
	 * @p writeImage.
	 **/
	void write( const QString& imageFile, bool deleteImage = true );
	virtual void writeImage( const QString& filename ) = 0;
	// vielleicht sollte man die 2. write-fkt einfach
	// iso-images schreiben lassen. Braucht man dann
	// noch ein TOC-file und kann man images auch track-at-once
	// schreiben?

	const QString& projectName() const { return m_projectName; }
	bool dao() const { return m_dao; }
	bool dummy() const { return m_dummy; }
	bool eject() const { return m_eject; }
	int speed() const { return m_speed; }
	K3bDevice* burner() const { return m_burner; }
	virtual int size() = 0;

	/**
	 * After result() has been emitted this returns the error-code
	 * to check the result.
	 **/
	int error() const;
	QString errorString() const;

	bool workInProgress() const;

public slots:
	void setDummy( bool d );
	void setDao( bool d );
	void setEject( bool e );
	void setSpeed( int speed );
	void setBurner( K3bDevice* dev );
	/** in the default implementation no canceled signal is emmited! */
	virtual void cancel();

protected slots:
	virtual void startRecording() = 0;
	virtual void parseCdrecordOutput( KProcess*, char* output, int len ) = 0;
	virtual void cdrecordFinished() = 0;

signals:
	void infoMessage( const QString& );
	void canceled();
	void result();
	void percent( int percent );
	void processedSize( unsigned long size );
	void timeLeft( const QTime& );

protected:
	void emitResult();
	void emitCanceled();
	virtual void emitProgress( unsigned long size, unsigned long processed, int speed = 0 );
	void emitMessage( const QString& msg );

	QString findTempFile( const QString& ending );

	/** path to cdrecord executable **/
	QString m_cdrecord;
	QTimer* m_timer;
	KProcess* m_process;
	
	int  m_error;
	
private:
	const QString m_projectName;
	K3bDevice* m_burner;
	bool m_dao;
	bool m_dummy;
	bool m_eject;
	int  m_speed;
}; // class K3bProject

#endif
