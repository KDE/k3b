/***************************************************************************
                          k3bcdrdaowriter.h  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg and
                                       klaus-Dieter Krannich
    email                : trueg@informatik.uni-freiburg.de
                           kd@math.tu-cottbus.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3B_CDRDAO_PARSER_H
#define K3B_CDRDAO_PARSER_H

#include <qstring.h>
#include <k3bjob.h>
#include <klocale.h>
#include <kdebug.h>
#include <qsocket.h>

#include <qdatetime.h>

struct ProgressMsg;


class K3bCdrdaoParser : public QObject
{
  Q_OBJECT

 public:
  K3bCdrdaoParser( QObject* parent = 0, const char* name = 0 );
  virtual ~K3bCdrdaoParser();
  void reinit();
  void parseCdrdaoLine( const QString& line );
  void parseCdrdaoWrote( const QString& line );
  void parseCdrdaoError( const QString& line ); 
  void parseCdrdaoMessage(QSocket *comSock);	  

 signals:
  void newSubTask( const QString& job );
  void infoMessage( const QString& msg, int type ); 
  void unknownCdrdaoLine( const QString& );
  void debuggingOutput( const QString&, const QString& );
  void percent( int p );
  void subPercent( int p );
  void buffer( int );
  void nextTrack( int, int );
  void processedSize( int, int );

 private:
  QTime m_startWriteTime;
  bool m_isStarted;
  int m_size;
  int m_currentTrack;
  struct ProgressMsg* m_oldMsg;
  struct ProgressMsg* m_newMsg;
};

#endif
