#include "k3bbootimageview.h"

#include "k3bdatadoc.h"
#include "k3bbootimage.h"
#include "k3bbootimagedialog.h"

#include <klocale.h>
#include <klistview.h>

#include <qpushbutton.h>
#include <qstring.h>


class K3bBootImageView::PrivateBootImageViewItem : public KListViewItem
{
public:
  PrivateBootImageViewItem( K3bBootImage* image, QListView* parent ) 
    : KListViewItem( parent ), 
      m_image( image ) {

  }

  PrivateBootImageViewItem( K3bBootImage* image, QListView* parent, QListViewItem* after )
    : KListViewItem( parent, after ),
      m_image( image ) {

  }

  QString text( int col ) const {
    if( col == 0 ) {
      switch( m_image->imageType ) {
      case K3bBootImage::FLOPPY:
	return i18n("Floppy");
      case K3bBootImage::HARDDISK:
	return i18n("Harddisk");
      }
    }
    else if( col == 1 )
      return m_image->fileItem->localPath();
    else
      return QString::null;
  }

  K3bBootImage* bootImage() const { return m_image; }

private:
  K3bBootImage* m_image;
};


K3bBootImageView::K3bBootImageView( K3bDataDoc* doc, QWidget* parent, const char* name )
  : base_K3bBootImageView( parent, name ),
    m_doc(doc)
{
  connect( buttonNew, SIGNAL(clicked()), 
	   this, SLOT(slotNewBootImage()) );
  connect( buttonEdit, SIGNAL(clicked()), 
	   this, SLOT(slotEditBootImage()) );
  connect( buttonDelete, SIGNAL(clicked()), 
	   this, SLOT(slotDeleteBootImage()) );

  updateBootImages();
}

K3bBootImageView::~K3bBootImageView()
{
}

void K3bBootImageView::slotNewBootImage()
{
  // TODO: maximum is 63

  K3bBootImageDialog d( 0, m_doc, this );
  if( d.exec() == KDialogBase::Ok )
    updateBootImages();
}


void K3bBootImageView::slotEditBootImage()
{
  QListViewItem* item = viewImages->selectedItem();
  if( item ) {
    K3bBootImageDialog d( ((PrivateBootImageViewItem*)item)->bootImage(), m_doc, this );
    d.exec();
  }
}


void K3bBootImageView::slotDeleteBootImage()
{
  QListViewItem* item = viewImages->selectedItem();
  if( item ) {
    K3bBootImage* i = ((PrivateBootImageViewItem*)item)->bootImage();
    delete item;
    m_doc->bootImages().removeRef( i );
  }
}


void K3bBootImageView::updateBootImages()
{
  viewImages->clear();
  for( QPtrListIterator<K3bBootImage> it( m_doc->bootImages() ); it.current(); ++it ) {
    (void)new PrivateBootImageViewItem( *it, viewImages, 
					viewImages->lastItem() );
  }
}

#include "k3bbootimageview.moc"
