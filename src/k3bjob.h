/***************************************************************************
                          k3bjob.h  -  description
                             -------------------
    begin                : Thu May 3 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BJOB_H
#define K3BJOB_H

#include <qobject.h>

class QString;
class K3bDoc;
class K3bDevice;
class KProcess;


/**This is the baseclass for all the jobs in K3b which actually do the work like burning a cd!
  *@author Sebastian Trueg
  */

class K3bJob : public QObject
{
  Q_OBJECT

 public:
  virtual ~K3bJob();

  virtual bool active() const { return false; }

  enum MessageType { STATUS, PROCESS, ERROR, INFO };

 protected:
  K3bJob( QObject* parent = 0, const char* name = 0 );

 public slots:
  virtual void start() = 0;
  virtual void cancel() = 0;

 signals:
  void infoMessage( const QString& msg, int type );
  void percent( int p );
  void subPercent( int p );
  void started();
  void canceled();
  void finished( bool success );
  void processedSize( int processed, int size );
  void processedSubSize( int processed, int size );
  void newTask( const QString& job );
  void newSubTask( const QString& job );
  void debuggingOutput(const QString&, const QString&);
};


class K3bBurnJob : public K3bJob
{
  Q_OBJECT
	
 public:
  K3bBurnJob( QObject* parent = 0 );
	
  virtual K3bDoc* doc() const { return 0; }
  virtual K3bDevice* writer() const { return 0; }

  /**
   * use K3b::WritingApp
   */
  int writingApp() const { return m_writeMethod; }

  /**
   * K3b::WritingApp "ored" together
   */
  virtual int supportedWritingApps() const;

 public slots:
  /**
   * use K3b::WritingApp
   */
  void setWritingApp( int w ) { m_writeMethod = w; }

 signals:
  void bufferStatus( int );

 protected slots:
  /**
   * calls parseCdrdaoLine
   * connect this to the cdrdao process
   * joines lines splitted by KProcess
   */
  void parseCdrdaoOutput( KProcess*, char* line, int len );

  /**
   * only reimplement this if the default parsing does not fit.
   * calls parseCdrdaoSpecialLine for every unparsed line
   * calls createCdrdaoProgress
   * calls startNewCdrdaoTrack
   * calls parseCdrdaoError
   */
  virtual void parseCdrdaoLine( const QString& line );

  /**
   * this should be reimplemented if some special line parsing is required
   */
  virtual void parseCdrdaoSpecialLine( const QString& line );

  virtual void parseCdrdaoError( const QString& line );

  virtual void createCdrdaoProgress( int made, int size );
  virtual void startNewCdrdaoTrack();

 private:
  QString m_notFinishedLine;
  int m_writeMethod;
};
#endif
