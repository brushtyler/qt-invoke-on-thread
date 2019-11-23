#ifndef TEST_H
#define TEST_H

#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QList>

class QThread;

class InvokeOnThreadTest : public QObject
{
    Q_OBJECT

private:
    QThread* mainThread;
    QThread* anotherThread;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testInvoke_data();
    void testInvoke();

    void testInvokeToQuitThreadsOnTasksDone();
};

#endif // TEST_H
