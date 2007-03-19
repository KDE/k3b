/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvideodvdrippingtitlelistview.h"
#include "k3bvideodvdrippingpreview.h"

#include <k3btooltip.h>
#include <k3bapplication.h>
#include <k3bmediacache.h>

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
#include <kiconloader.h>
#include <kapplication.h>


static QString audioStreamString( const K3bVideoDVD::Title& title, unsigned int maxLines = 9999, bool includeExtInfo = true )
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
      .arg( includeExtInfo && title.audioStream(i).codeExtension() != K3bVideoDVD::AUDIO_CODE_EXT_UNSPECIFIED 
	    ? QString(" ") + K3bVideoDVD::audioCodeExtensionString( title.audioStream(i).codeExtension() )
	    : QString::null );
  }
  if( title.numAudioStreams() > maxLines )
    s += "...";

  return s;
}


static QString subpictureStreamString( const K3bVideoDVD::Title& title, unsigned int maxLines = 9999, bool includeExtInfo = true )
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
      .arg( includeExtInfo && title.subPictureStream(i).codeExtension() != K3bVideoDVD::SUBPIC_CODE_EXT_UNSPECIFIED 
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
    setMarginHorizontal( 5, 2 );
    setChecked(true);

    m_previewSet = false;
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

  void setPreview( const QImage& preview ) {
    m_preview = preview;
    m_scaledPreview = QPixmap();

    m_previewSet = true;

    repaint();
  }

  const QImage& preview() const {
    return m_preview;
  }

protected:
  void paintK3bCell( QPainter* p, const QColorGroup& cg, int col, int w, int align ) {
    p->save();

    if( col == 0 ) {
      // the check mark
      K3bCheckListViewItem::paintK3bCell( p, cg, col, w, align );
    }
    else if( col == 2 ) {
      if( isSelected() ) {
	p->fillRect( 0, 0, w, height(),
		     cg.brush( QColorGroup::Highlight ) );
	p->setPen( cg.highlightedText() );
      }
      else {
	p->fillRect( 0, 0, w, height(), cg.base() ); 
	p->setPen( cg.text() );
      }

      // draw the preview
      int h = height();
      h -= 2*marginVertical();
      h -= 1; // the separator
      if( !m_preview.isNull() ) {
	if( m_scaledPreview.height() != h ) {
	  // recreate scaled preview
	  int preH = m_preview.height()*w/m_preview.width();
	  int preW = m_preview.width()*h/m_preview.height();
	  if( preH > h )
	    preH = m_preview.height()*preW/m_preview.width();
	  if( preW > w )
	    preW = m_preview.width()*preH/m_preview.height();
	  m_scaledPreview.convertFromImage( m_preview.smoothScale( preW, preH ), 0 );
	}

	// center the preview in the column
	int yPos = ( height() - m_scaledPreview.height() ) / 2;
	int xPos = ( w - m_scaledPreview.width() ) / 2;

	p->drawPixmap( xPos, yPos, m_scaledPreview );
      }
      else if( m_previewSet ) {
	int preW = 0;
	if( m_title.videoStream().displayAspectRatio()	== K3bVideoDVD::VIDEO_ASPECT_RATIO_4_3 )
	  preW = h*4/3;
	else
	  preW = h*16/9;

	p->drawRect( ( w - preW ) / 2, ( height() - h ) / 2, preW, h );
	QPixmap noIcon = KApplication::kApplication()->iconLoader()->loadIcon( "no", KIcon::NoGroup, KIcon::SizeSmall, KIcon::DefaultState, 0, true );
	p->drawPixmap( ( w - noIcon.width() ) / 2, ( height() - noIcon.height() ) / 2, noIcon );
      }
      else {
	p->drawText( 0, 0, w, height(), Qt::AlignCenter, "..." );
      }
    }
    else {
      QString s = text( col );
      if( s.isEmpty() )
	K3bCheckListViewItem::paintK3bCell( p, cg, col, w, align );
      else {
	QColorGroup cg1( cg );
	if( isSelected() ) {
	  p->fillRect( 0, 0, w, height(),
		       cg.brush( QColorGroup::Highlight ) );
	  cg1.setColor( QColorGroup::Text, cg.highlightedText() );
	}
	else {
	  p->fillRect( 0, 0, w, height(), cg.base() ); 
	}

	// paint using QSimpleRichText
	QSimpleRichText rt( text(col), listView()->font() );
	rt.setWidth( 600 ); // way to big to avoid line breaks
	// normally we would have to clip the height to height()-2*marginVertical(). But if we do that
	// some characters are cut (such as p or q). It seems as if QSimpleRichText does not properly 
	// calculate it's height...
	rt.draw( p, 0, marginVertical(), QRect( 0, 0, w, height() ), cg1 );
      }
    }

    // draw the separator
    if( listView()->firstChild() != this ) {
      p->translate( -1*marginHorizontal(col), 0 );
      // FIXME: modify the value from palette().disabled().foreground() to be lighter (or darker, depending on the background color )
      p->setPen( Qt::lightGray );
      p->drawLine( 0, 0, w+2*marginHorizontal(col), 0 );
    }

    p->restore();
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

    case 3:
      // video stream info
      return QString("<p>%1 %2x%3<br>%4%5")
	.arg( m_title.videoStream().mpegVersion() == 0 ? i18n("MPEG1") : i18n("MPEG2") )
	.arg( m_title.videoStream().pictureWidth() )
	.arg( m_title.videoStream().pictureHeight() )
	.arg( m_title.videoStream().displayAspectRatio() == K3bVideoDVD::VIDEO_ASPECT_RATIO_4_3 ? "4:3" : "16:9" )
	.arg( m_title.videoStream().letterboxed() ? QString(" - <em>") + i18n("letterboxed") + QString("</em>"): 
	      m_title.videoStream().permittedDf() == K3bVideoDVD::VIDEO_PERMITTED_DF_LETTERBOXED 
	      ? QString(" - <em>") + i18n("anamorph") + QString("</em>") : QString::null );

    case 4:
      // audio streams info
      if( m_title.numAudioStreams() > 0 )
	return audioStreamString( m_title, 2, false );
      else
	return "<p><small><em>" + i18n("No audio streams") + "</em>";

    case 5:
      // subpicture streams info
      if( m_title.numSubPictureStreams() > 0 )
	return subpictureStreamString( m_title, 2, false );
      else
	return "<p><small><em>" + i18n("No Subpicture streams") + "</em>";

    default:
      return K3bCheckListViewItem::text( col );
    }
  }

  K3bVideoDVD::Title m_title;

  bool m_previewSet;
  QImage m_preview;
  QPixmap m_scaledPreview;
};


class K3bVideoDVDRippingTitleListView::TitleToolTip : public K3bToolTip
{
public:
  TitleToolTip( K3bVideoDVDRippingTitleListView* view )
    : K3bToolTip( view->viewport() ),
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
    case 2:
      if( !item->preview().isNull() ) {
	QPixmap previewPix;
	if( previewPix.convertFromImage( item->preview() ) )
	  tip( r, previewPix, 0 );
      }
      break;
    case 4:
      if( item->videoDVDTitle().numAudioStreams() > 0 )
	tip( r, "<p><b>" + i18n("Audio Streams") + "</b><p>" + audioStreamString( item->videoDVDTitle() ), 0 );
      break;
    case 5:
      if( item->videoDVDTitle().numSubPictureStreams() > 0 )
	tip( r, "<p><b>" + i18n("Subpicture Streams") + "</b><p>" + subpictureStreamString( item->videoDVDTitle() ), 0 );
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
  addColumn( i18n("Preview") );
  addColumn( i18n("Video") );
  addColumn( i18n("Audio") );
  addColumn( i18n("Subpicture") );

  header()->setClickEnabled( false );
  setColumnWidthMode( 0, QListView::Manual );
  setColumnWidth( 0, 20 );
  header()->setResizeEnabled( false, 0 );

  m_toolTip = new TitleToolTip( this );

  m_previewGen = new K3bVideoDVDRippingPreview( this );
  connect( m_previewGen, SIGNAL(previewDone(bool)),
	   this, SLOT(slotPreviewDone(bool)) );
}


K3bVideoDVDRippingTitleListView::~K3bVideoDVDRippingTitleListView()
{
  delete m_toolTip;
}


void K3bVideoDVDRippingTitleListView::setVideoDVD( const K3bVideoDVD::VideoDVD& dvd )
{
  clear();

  m_dvd = dvd;
  m_medium = k3bappcore->mediaCache()->medium( m_dvd.device() );
  m_itemMap.resize( dvd.numTitles() );

  for( unsigned int i = 0; i < dvd.numTitles(); ++i )
    m_itemMap[i] = new TitleViewItem( this, lastItem(), dvd.title(i) );

  m_currentPreviewTitle = 1;
  m_previewGen->generatePreview( m_dvd, 1 );
}


void K3bVideoDVDRippingTitleListView::slotPreviewDone( bool success )
{
  if( success )
    m_itemMap[m_currentPreviewTitle-1]->setPreview( m_previewGen->preview() );
  else
    m_itemMap[m_currentPreviewTitle-1]->setPreview( QImage() );

  // cancel if we got hidden or if the medium changed.
  if( isVisible() && m_medium == k3bappcore->mediaCache()->medium( m_dvd.device() ) ) {
    ++m_currentPreviewTitle;
    if( m_currentPreviewTitle <= m_dvd.numTitles() )
      m_previewGen->generatePreview( m_dvd, m_currentPreviewTitle );
  }
}


void K3bVideoDVDRippingTitleListView::hideEvent( QHideEvent* e )
{
  //
  // For now we do it the easy way: just stop the preview generation
  // once this view is hidden
  //
  m_previewGen->cancel();

  K3bListView::hideEvent( e );
}

#include "k3bvideodvdrippingtitlelistview.moc"
