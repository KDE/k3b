
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
class QTextView;
class K3bDevice;
class K3bBlankingJob;



class K3bBlankingDialog : public KDialogBase
{
Q_OBJECT

 public:
  K3bBlankingDialog( QWidget*, const char* );
  ~K3bBlankingDialog();

 protected slots:
  void slotRefreshWriterSpeeds();
  void slotUser1();
  void slotUser2();
  void slotCancel();
  void slotInfoMessage( const QString& );
  void slotJobFinished();

 protected:
  void closeEvent( QCloseEvent* );

 private:
  void setupGui();
  int writerSpeed() const;
  K3bDevice* writerDevice() const;
  
  QGroupBox* m_groupWriter;
  QComboBox* m_comboSpeed;
  QComboBox* m_comboWriter;

  QButtonGroup* m_groupBlankType;
  QRadioButton* m_radioCompleteBlank;
  QRadioButton* m_radioFastBlank;
  QRadioButton* m_radioBlankTrack;
  QRadioButton* m_radioUncloseSession;
  QRadioButton* m_radioBlankSession;

  QGroupBox* m_groupOptions;
  QCheckBox* m_checkForce;

  QGroupBox* m_groupOutput;
  QTextView* m_viewOutput;

  K3bBlankingJob* m_job;
};

#endif
