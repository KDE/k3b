
#ifndef K3B_BURNING_OPTION_TAB_H
#define K3B_BURNING_OPTION_TAB_H

#include <qwidget.h>

class QCheckBox;
class QLabel;
class QGroupBox;
class QComboBox;
class QString;
class KIntNumInput;


class K3bBurningOptionTab : public QWidget
{
Q_OBJECT

 public:
  K3bBurningOptionTab( QWidget* parent = 0, const char* name = 0 );
  ~K3bBurningOptionTab();

  void saveSettings();
  void readSettings();

 private slots:
  void slotChangePregapFormat( const QString& );
  void slotSetDefaultBufferSizes( bool );

 private:
  void setupGui();

  QGroupBox*    m_groupData;
  QGroupBox*    m_groupAudio;
  QGroupBox*    m_groupAdvanced;

  QCheckBox*    m_checkUseID3Tag;

  QComboBox*    m_comboPregapFormat;
  KIntNumInput* m_editDefaultPregap;
  bool          m_bPregapSeconds;

  QCheckBox*    m_checkEject;
  QCheckBox*    m_checkManualWritingBufferSize;
  KIntNumInput* m_editWritingBufferSizeCdrecord;
  KIntNumInput* m_editWritingBufferSizeCdrdao;
  QCheckBox*    m_checkAllowWritingAppSelection;
};


#endif
