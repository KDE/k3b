#ifndef K3B_ISO_9660_TEST_H
#define K3B_ISO_9660_TEST_H

#include <QObject>

class Iso9660Test : public QObject
{
    Q_OBJECT

private slots:
    void testEntries_data();
    void testEntries();
};

#endif // K3B_ISO_9660_TEST_H
