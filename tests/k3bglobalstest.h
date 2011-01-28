
#ifndef K3B_GLOBALS_TEST_H
#define K3B_GLOBALS_TEST_H

#include <QtCore/QObject>

class GlobalsTest : public QObject
{
    Q_OBJECT
public:
    GlobalsTest();
private slots:
    void testCutFilename();
    void testRemoveFilenameExtension();
};

#endif // K3B_GLOBALS_TEST_H
