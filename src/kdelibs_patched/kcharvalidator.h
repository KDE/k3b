
#ifndef KCHARVALIDATOR_H
#define KCHARVALIDATOR_H


#include <qvalidator.h>


/**
 * A Validator with two modes (@ref #validationMode):
 * 1. ValidChars   the validator accepts every character set in the contructor or
 * in @ref #setChars()
 * 2. InvalidChars the validator accepts all characters except the ones set in the
 * constructor or in @ref #setChars()
 *
 * @short A Validator for any set of characters
 * @author Sebastian Trueg <sebastian@trueg.de>
 * @version 0.0.1
 */
class KCharValidator : public QValidator
{
  Q_OBJECT
  Q_PROPERTY( QString chars READ chars WRITE setChars )
  Q_PROPERTY( ValidationMode mode READ validationMode WRITE setValidationMode )
  Q_ENUMS( ValidationMode )

 public:
  enum ValidationMode { ValidChars, InvalidChars };
  
  /**
   * Creates a new KCharValidator.
   */
  KCharValidator( QWidget* parent, const char* name = 0, 
		  const QString& chars = QString::null, 
		  ValidationMode mode = ValidChars );
  ~KCharValidator();
  
  /**
   * Reimplemented from QValidator.
   */
  QValidator::State validate( QString &, int & ) const;

  /**
   * Set the used characters
   */
  void setChars( const QString& chars );

  void setValidationMode( ValidationMode mode );

  const QString& chars() const;

  ValidationMode validationMode() const;

 private:
  QString m_qsChars;
  // since we use QRegExp the following characters have to be escaped: \ ] - ^
  // this is done in setChars()
  QString m_qsEscapedChars;
  ValidationMode m_mode;
};


#endif
