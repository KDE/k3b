#ifndef K3BDATAVIEWITEM_H
#define K3BDATAVIEWITEM_H

#include <klistview.h>

class K3bDataItem;
class K3bFileItem;
class K3bDirItem;
class K3bDataDoc;

class QPainter;
class QColorGroup;



class K3bDataViewItem : public KListViewItem
{
 public:
  K3bDataViewItem( QListView* parent );
  K3bDataViewItem( QListViewItem* parent );
  ~K3bDataViewItem();
	
  virtual K3bDataItem* dataItem() const { return 0; }

  virtual void paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align );
};


class K3bDataDirViewItem : public K3bDataViewItem
{
 public:
  K3bDataDirViewItem( K3bDirItem* dir, QListView* parent );
  K3bDataDirViewItem( K3bDirItem* dir, QListViewItem* parent );
  ~K3bDataDirViewItem();
	
  virtual QString text( int ) const;
	
  /** reimplemented from QListViewItem */
  void setText(int col, const QString& text );

  K3bDirItem* dirItem() const { return m_dirItem; }
  K3bDataItem* dataItem() const;

  /**
   * reimplemented to have directories always sorted before files
   */
  QString key( int, bool ) const;

 private:
  K3bDirItem* m_dirItem;
};


class K3bDataFileViewItem : public K3bDataViewItem
{
 public:
  K3bDataFileViewItem( K3bFileItem*, QListView* parent );
  K3bDataFileViewItem( K3bFileItem*, QListViewItem* parent );
  ~K3bDataFileViewItem() {}
	
  QString text( int ) const;

  /** reimplemented from QListViewItem */
  void setText(int col, const QString& text );

  K3bFileItem* fileItem() const { return m_fileItem; }
  K3bDataItem* dataItem() const;

  /**
   * reimplemented to have directories always sorted before files
   */
  QString key( int, bool ) const;
	
 private:
  K3bFileItem* m_fileItem;
};


class K3bDataRootViewItem : public K3bDataDirViewItem
{
 public:
  K3bDataRootViewItem( K3bDataDoc*, QListView* parent );
  ~K3bDataRootViewItem() {}
	
  QString text( int ) const;
	
  /** reimplemented from QListViewItem */
  void setText(int col, const QString& text );
		
 private:
  K3bDataDoc* m_doc;
};

#endif
