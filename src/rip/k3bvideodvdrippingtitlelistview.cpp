/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvideodvdrippingtitlelistview.h"

#include <k3bvideodvd.h>
#include <k3bvideodvdaudiostream.h>
#include <k3bvideodvdvideostream.h>
#include <k3bvideodvdsubpicturestream.h>

#include <qsimplerichtext.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qheader.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kglobal.h>


static QString audioStreamString( const K3bVideoDVD::Title& title, unsigned int maxLines = 9999 )
{
  QString s = "<p>";
  for( unsigned int i = 0; i < QMIN( title.numAudioStreams(), maxLines ); ++i ) {
    if( i > 0 )
      s += "<br>";
    s += QString::number(i+1) + ": " 
      + i18n("%1 %2Ch (%3<em>%4</em>)")
      .arg( K3bVideoDVD::audioFormatString( title.audioStream(i).format() ) )
      .arg( title.audioStream(i).channels() )
      .arg( title.audioStream(i).langCode().isEmpty()
	    ? i18n("unknown language")
	    : KGlobal::locale()->twoAlphaToLanguageName( title.audioStream(i).langCode() ) )
      .arg( title.audioStream(i).codeExtension() != K3bVideoDVD::AUDIO_CODE_EXT_UNSPECIFIED 
	    ? QString(" ") + K3bVideoDVD::audioCodeExtensionString( title.audioStream(i).codeExtension() )
	    : QString::null );
  }
  if( title.numAudioStreams() > maxLines )
    s += "...";

  return s;
}


static QString subpictureStreamString( const K3bVideoDVD::Title& title, unsigned int maxLines = 9999 )
{
  QString s = "<p>";
  for( unsigned int i = 0; i < QMIN( title.numSubPictureStreams(), maxLines ); ++i ) {
    if( i > 0 )
      s += "<br>";
    s += QString::number(i+1) + ": " 
      + QString("%1 (%2<em>%3</em>)")
      .arg( title.subPictureStream(i).codeMode() == K3bVideoDVD::SUBPIC_CODE_MODE_RLE 
	    ? i18n("RLE")
	    : i18n("Extended") )
      .arg( title.subPictureStream(i).langCode().isEmpty()
	    ? i18n("unknown language")
	    : KGlobal::locale()->twoAlphaToLanguageName( title.subPictureStream(i).langCode() ) )
      .arg( title.subPictureStream(i).codeExtension() != K3bVideoDVD::SUBPIC_CODE_EXT_UNSPECIFIED 
	    ? QString(" ") + K3bVideoDVD::subPictureCodeExtensionString( title.subPictureStream(i).codeExtension() )
	    : QString::null );
  }
  if( title.numSubPictureStreams() > maxLines )
    s += "...";

  return s;
}



class K3bVideoDVDRippingTitleListView::TitleViewItem : public K3bCheckListViewItem
{
public:
  TitleViewItem( K3bVideoDVDRippingTitleListView* parent, QListViewItem* after, const K3bVideoDVD::Title& title ) 
    : K3bCheckListViewItem( parent, after ),
      m_title( title ) {

    setMarginVertical( 4 );
    setMarginHorizontal( 1, 2 );
    setMarginHorizontal( 2, 2 );
    setMarginHorizontal( 3, 2 );
    setMarginHorizontal( 4, 2 );
    setChecked(true);
  }

  const K3bVideoDVD::Title& videoDVDTitle() const { return m_title; }

  void setup() {
    widthChanged();    

    // set a valid height
    int maxH = 0;
    for( int c = 1; c <= 4; ++c ) {
      QSimpleRichText rt( text(c), listView()->font() );
      rt.setWidth( 600 ); // way to big to avoid line breaks
      maxH = QMAX( maxH, rt.height() );
    }

    setHeight( maxH + 2*marginVertical() );
  }

  int width( const QFontMetrics& fm, const QListView* lv, int c ) const {
    if( c == 0 )
      return K3bCheckListViewItem::width( fm, lv, c );
    else {
      QSimpleRichText rt( text(c), lv->font() );
      rt.setWidth( 600 ); // way to big to avoid line breaks
      return rt.widthUsed() + 2*marginHorizontal( c );
    }
  }

protected:
  void paintK3bCell( QPainter* p, const QColorGroup& cg, int col, int width, int align ) {
    if( col == 0 ) {
      // the check mark
      K3bCheckListViewItem::paintK3bCell( p, cg, col, width, align );
    }
    else {
      QString s = text( col );
      if( s.isEmpty() )
	K3bCheckListViewItem::paintK3bCell( p, cg, col, width, align );
      else {
	if( isSelected() ) {
	  p->fillRect( 0, 0, width, height(),
		       cg.brush( QColorGroup::Highlight ) );
	  p->setPen( cg.highlightedText() );
	}
	else {
	  p->fillRect( 0, 0, width, height(), cg.base() ); 
	  p->setPen( cg.text() );
	}

	// paint using QSimpleRichText
	QSimpleRichText rt( text(col), listView()->font() );
	rt.setWidth( 600 ); // way to big to avoid line breaks
	// normally we would have to clip the height to height()-2*marginVertical(). But if we do that
	// some characters are cut (such as p or q). It seems as if QSimpleRichText does not properly 
	// calculate it's height...
	rt.draw( p, 0, marginVertical(), QRect( 0, 0, width, height() ), cg );
      }
    }

    // draw the separator
    if( listView()->firstChild() != this ) {
      p->save();
      p->translate( -1*marginHorizontal(col), 0 );
      // FIXME: modify the value from palette().disabled().foreground() to be lighter (or darker, depending on the background color )
      p->setPen( Qt::lightGray );
      p->drawLine( 0, 0, width+2*marginHorizontal(col), 0 );
      p->restore();
    }
  }

private:
  QString text( int col ) const {
    switch( col ) {
    case 1:
      // Title X + length
      return i18n("<p><b>Title %1 (%2)</b><br>"
		  "%3")
	.arg( m_title.titleNumber(), 2 )
	.arg( m_title.playbackTime().toString( false ) )
	.arg( i18n("%n chapter", "%n chapters", m_title.numPTTs() ) );

    case 2:
      // video stream info
      return QString("<p>%1 %2x%3<br>%4%5")
	.arg( m_title.videoStream().mpegVersion() == 0 ? i18n("MPEG1") : i18n("MPEG2") )
	.arg( m_title.videoStream().pictureWidth() )
	.arg( m_title.videoStream().pictureHeight() )
	.arg( m_title.videoStream().displayAspectRatio() == K3bVideoDVD::VIDEO_ASPECT_RATIO_4_3 ? "4:3" : "16:9" )
	.arg( m_title.videoStream().letterboxed() ? QString(" - <em>") + i18n("letterboxed") + QString("</em>"): 
	      m_title.videoStream().permittedDf() == K3bVideoDVD::VIDEO_PERMITTED_DF_LETTERBOXED 
	      ? QString(" - <em>") + i18n("anamorph") + QString("</em>") : QString::null );

    case 3:
      // audio streams info
      if( m_title.numAudioStreams() > 0 )
	return audioStreamString( m_title, 2 );
      else
	return "<p><small><em>" + i18n("No audio streams") + "</em>";

    case 4:
      // subpicture streams info
      if( m_title.numSubPictureStreams() > 0 )
	return subpictureStreamString( m_title, 2 );
      else
	return "<p><small><em>" + i18n("No Subpicture streams") + "</em>";

    default:
      return QString::null;
    }
  }

  K3bVideoDVD::Title m_title;
};


class K3bVideoDVDRippingTitleListView::TitleToolTip : public QToolTip
{
public:
  TitleToolTip( K3bVideoDVDRippingTitleListView* view )
    : QToolTip( view->viewport() ),
      m_view( view ) {
  }

  void maybeTip( const QPoint& pos ) {
    TitleViewItem* item = static_cast<TitleViewItem*>( m_view->itemAt( pos ) );
    QPoint contentsPos = m_view->viewportToContents( pos );
    if( !item )
      return;
    int col = m_view->header()->sectionAt( contentsPos.x() );

    QRect r = m_view->itemRect( item );
    int headerPos = m_view->header()->sectionPos( col );
    r.setLeft( headerPos );
    r.setRight( headerPos + m_view->header()->sectionSize( col ) );

    switch( col ) {
    case 3:
      if( item->videoDVDTitle().numAudioStreams() > 2 )
	tip( r, "<p><b>" + i18n("Audio Streams") + "</b><p>" + audioStreamString( item->videoDVDTitle() ) );
      break;
    case 4:
      if( item->videoDVDTitle().numSubPictureStreams() > 2 )
	tip( r, "<p><b>" + i18n("Subpicture Streams") + "</b><p>" + subpictureStreamString( item->videoDVDTitle() ) );
      break;
    }
  }

private:
  K3bVideoDVDRippingTitleListView* m_view;
};



K3bVideoDVDRippingTitleListView::K3bVideoDVDRippingTitleListView( QWidget* parent )
  : K3bListView( parent )
{
  setFullWidth(true);
  setSorting(-1);
  setAllColumnsShowFocus( true );
  setSelectionModeExt( Single );

  addColumn( "" );
  addColumn( i18n("Title") );
  addColumn( i18n("Video") );
  addColumn( i18n("Audio") );
  addColumn( i18n("Subpicture") );

  header()->setClickEnabled( false );
  setColumnWidthMode( 0, QListView::Manual );
  setColumnWidth( 0, 20 );
  header()->setResizeEnabled( false, 0 );

  m_toolTip = new TitleToolTip( this );
}


K3bVideoDVDRippingTitleListView::~K3bVideoDVDRippingTitleListView()
{
  delete m_toolTip;
}


void K3bVideoDVDRippingTitleListView::setVideoDVD( const K3bVideoDVD::VideoDVD& dvd )
{
  clear();

  for( unsigned int i = 0; i < dvd.numTitles(); ++i ) {
    (void)new TitleViewItem( this, lastItem(), dvd.title(i) );
  }
}


#include "k3bvideodvdrippingtitlelistview.moc"
