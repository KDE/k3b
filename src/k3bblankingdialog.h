
#ifndef K3B_BLANKING_DIALOG_H
#define K3B_BLANKING_DIALOG_H

#include <kdialogbase.h>

class QString;
class QGroupBox;
class QComboBox;
class QButtonGroup;
class QRadioButton;
class QCheckBox;
class QComboBox;
class QCloseEvent;
class KListView;
class K3bDevice;
class K3bBlankingJob;
class KProgress;
class K3bWriterSelectionWidget;


class K3bBlankingDialog : public KDialogBase
{
Q_OBJECT

 public:
  K3bBlankingDialog( QWidget*, const char* );
  ~K3bBlankingDialog();

 protected slots:
  void slotUser1();
  void slotUser2();
  void slotInfoMessage( const QString& msg, int type );
  void slotJobFinished();

 protected:
  void closeEvent( QCloseEvent* );

 private:
  void setupGui();
  
  K3bWriterSelectionWidget* m_writerSelectionWidget;

  QButtonGroup* m_groupBlankType;
  QRadioButton* m_radioCompleteBlank;
  QRadioButton* m_radioFastBlank;
  QRadioButton* m_radioBlankTrack;
  QRadioButton* m_radioUncloseSession;
  QRadioButton* m_radioBlankSession;

  QGroupBox* m_groupOptions;
  QCheckBox* m_checkForce;

  QGroupBox* m_groupOutput;
  KListView* m_viewOutput;

  K3bBlankingJob* m_job;
};

#endif
