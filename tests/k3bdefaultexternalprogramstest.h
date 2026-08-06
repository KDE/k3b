#ifndef K3B_DEFAULT_EXTERNAL_PROGRAMS_TEST_H
#define K3B_DEFAULT_EXTERNAL_PROGRAMS_TEST_H

#include <QObject>

class DefaultExternalProgramsTest : public QObject
{
    Q_OBJECT

private slots:
    void testContainsWord_data();
    void testContainsWord();
};

#endif // K3B_DEFAULT_EXTERNAL_PROGRAMS_TEST_H
