
#include "kcharvalidator.h"

#include <qregexp.h>


KCharValidator::KCharValidator( QWidget* parent, const char* name,
				const QString& chars,
				KCharValidator::ValidationMode mode )
  : QValidator( parent, name )
{
  setChars( chars );
  m_mode = mode;
}


KCharValidator::~KCharValidator()
{
}


QValidator::State KCharValidator::validate( QString& input, int& ) const
{
  QRegExp exp;

  if( m_mode == KCharValidator::InvalidChars )
    exp.setPattern( QString("[%1]").arg(m_qsEscapedChars) );
  else
    exp.setPattern( QString("[^%1]").arg(m_qsEscapedChars) );

  if( exp.find( input, 0 ) == -1 )
    return QValidator::Acceptable;
  else
    return QValidator::Invalid;
}


void KCharValidator::setChars( const QString& chars )
{
  // we need to escape all occurences of '\', ']', '-', and '^' because these
  // are special characters with a character set ([...]) of QRegExp
  int pos = 0;
  m_qsChars = chars;
  m_qsEscapedChars = chars;
  while( pos != -1 && pos < (int)m_qsEscapedChars.length() ) {
    pos = m_qsEscapedChars.find( '\\', pos );
    if( pos != -1 ) {
      m_qsEscapedChars.replace( pos, 1, "\\\\" );
      pos += 2;
    }
  }
  pos = 0;
  while( pos != -1 && pos < (int)m_qsEscapedChars.length() ) {
    pos = m_qsEscapedChars.find( ']', pos );
    if( pos != -1 ) {
      m_qsEscapedChars.replace( pos, 1, "\\]" );
      pos += 2;
    }
  }
  pos = 0;
  while( pos != -1 && pos < (int)m_qsEscapedChars.length() ) {
    pos = m_qsEscapedChars.find( '-', pos );
    if( pos != -1 ) {
      m_qsEscapedChars.replace( pos, 1, "\\-" );
      pos += 2;
    }
  }
  pos = 0;
  while( pos != -1 && pos < (int)m_qsEscapedChars.length() ) {
    pos = m_qsEscapedChars.find( '^', pos );
    if( pos != -1 ) {
      m_qsEscapedChars.replace( pos, 1, "\\^" );
      pos += 2;
    }
  }
}


void KCharValidator::setValidationMode( KCharValidator::ValidationMode mode )
{
  m_mode = mode;
}


const QString& KCharValidator::chars() const
{
  return m_qsChars;
}


KCharValidator::ValidationMode KCharValidator::validationMode() const
{
  return m_mode;
}

