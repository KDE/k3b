#ifndef K3B_MIXED_DOC_H
#define K3B_MIXED_DOC_H

#include "../k3bdoc.h"

class K3bAudioDoc;
class K3bDataDoc;
class QDomDocument;
class K3bBurnJob;
class K3bView;
class QWidget;


class K3bMixedDoc : public K3bDoc
{
  Q_OBJECT

 public: 
  K3bMixedDoc( QObject* parent = 0 );
  ~K3bMixedDoc();

  bool newDocument();

  unsigned long long size() const;
  unsigned long long length() const;

  K3bView* newView( QWidget* parent );

  int numOfTracks() const;

  K3bBurnJob* newBurnJob();

  K3bAudioDoc* audioDoc() const { return m_audioDoc; }
  K3bDataDoc* dataDoc() const { return m_dataDoc; }

 public slots:
  void addUrl( const KURL& url );
  void addUrls( const KURL::List& urls );

 protected:
  bool loadDocumentData( QDomDocument* );
  bool saveDocumentData( QDomDocument* );
  QString documentType() const { return "k3b_mixed_project"; }
  
  void loadDefaultSettings();

 private:
  K3bDataDoc* m_dataDoc;
  K3bAudioDoc* m_audioDoc;
};


#endif
