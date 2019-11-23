#include "test.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QDebug>
#include "../invokeonthread.h"
#include "a.h"


void InvokeOnThreadTest::initTestCase()
{
    mainThread = QThread::currentThread();
    mainThread->setObjectName("mainThread");

    // create another threads
    anotherThread = new QThread();
    anotherThread->setObjectName("anotherThread");
    anotherThread->start();
}

void InvokeOnThreadTest::cleanupTestCase()
{
    anotherThread->quit();
    anotherThread->wait();
    delete anotherThread;
}


using Func = void(QThread*, A*, int);

void invokeMethod(QThread *t, A *a_ptr, int val)
{
    MetaObject::invokeMethod(t, a_ptr, &A::setValue, val);
}

void updateValue(A *b, int newval)
{
    b->setValue(newval);
}

void invokeFunction(QThread *t, A *a_ptr, int val)
{
    MetaObject::invokeFunction(t, &updateValue, a_ptr, val);
}

void invokeLambda(QThread *t, A *a_ptr, int val)
{
    MetaObject::invokeFunction(t, [](A* b, int newval){ b->setValue(newval); }, a_ptr, val);
}

void invokeLambdaWithCapture(QThread *t, A *a_ptr, int val)
{
    MetaObject::invokeFunction(t, [a_ptr](int newval){ a_ptr->setValue(newval); }, val);
}

void InvokeOnThreadTest::testInvoke_data()
{
    /* Hack to use a function as QTest column type:
     * convert it to void*, then reinterpret it back to Func* when needed.
     * This is required because Func is not a registered metatype.
     */
    QTest::addColumn<void*>("functionPointer");
    QTest::addColumn<QThread*>("runOnThread");
    QTest::addColumn<int>("newValue");

    QTest::newRow("method on same thread")
            << reinterpret_cast<void*>(&invokeMethod)
            << mainThread
            << 10;
    QTest::newRow("method on different thread")
            << reinterpret_cast<void*>(&invokeMethod)
            << anotherThread
            << 20;

    QTest::newRow("function on same thread")
            << reinterpret_cast<void*>(&invokeFunction)
            << mainThread
            << 30;
    QTest::newRow("function on different thread")
            << reinterpret_cast<void*>(&invokeFunction)
            << anotherThread
            << 40;

    QTest::newRow("lambda on same thread")
            << reinterpret_cast<void*>(&invokeLambda)
            << mainThread
            << 50;
    QTest::newRow("lambda on different thread")
            << reinterpret_cast<void*>(&invokeLambda)
            << anotherThread
            << 60;

    QTest::newRow("lambda w/ capture on same thread")
            << reinterpret_cast<void*>(&invokeLambdaWithCapture)
            << mainThread
            << 70;
    QTest::newRow("lambda w/ capture on different thread")
            << reinterpret_cast<void*>(&invokeLambdaWithCapture)
            << anotherThread
            << 80;
}

void InvokeOnThreadTest::testInvoke()
{
    QFETCH(void*, functionPointer);
    QFETCH(QThread*, runOnThread);
    QFETCH(int, newValue);

    A a;
    QVERIFY(0 == a.getValue());

    // connect to A::valueChanged to check thread it's running on
    QMetaObject::Connection conn = QObject::connect(&a, &A::valueChanged, [newValue, runOnThread](int value){
        qDebug() << "got update: new value is" << value << "=> on thread" << QThread::currentThread()->objectName();
        QVERIFY(QThread::currentThread() == runOnThread);
        QVERIFY(value == newValue);
    });

    QSignalSpy spy(&a, &A::valueChanged);

    // execute the func (passed as void*)
    Func *func = reinterpret_cast<Func*>(functionPointer);
    func(runOnThread, &a, newValue);

    QVERIFY(spy.count() > 0 || spy.wait());
    QVERIFY(a.getValue() == newValue);

    QObject::disconnect(conn);
}


void doTask(uint ms, bool *done)
{
    QThread::msleep(ms);
    *done = true;
}

void InvokeOnThreadTest::testInvokeToQuitThreadsOnTasksDone()
{
    const int threadCount = 10;
    const int taskPerThread = 10;
    const int taskDurationMs = 100;
    const int taskCount = taskPerThread * threadCount;

    // create and start threads
    QList<QThread*> threadList;
    for (int i = 0; i < threadCount; i++)
    {
        QThread *t = new QThread();
        t->setObjectName(QString("thread_%1").arg(i));
        t->start();
        threadList << t;
    }

    // create vector to check tasks status
    QVector<bool> tasksDone(taskCount);
    for (int i = 0; i < taskCount; i++)
    {
        tasksDone[i] = false;
    }

    // enqueue tasks on threads
    for (int i = 0; i < taskCount; i++)
    {
        QThread *t = threadList.at(i % threadCount);
        MetaObject::invokeFunction(t, &doTask, taskDurationMs, &tasksDone[i]);
    }

    // quit threads after tasks have finished
    for (int i = 0; i < threadCount; i++)
    {
        QThread *t = threadList.at(i);
        /* We must quit the thread after all the enqueued tasks on that thread have finished:
         *     i.e. invokeFunction(t, ...)
         * where ... is the part that quits the thread.
         *
         * Since the thread is a QObject that lives on main thread,
         * we enforce invokation of t->quit() on that thread:
         *     i.e. invokeMethod(t->thread(), t, &QThread::quit)
         */
        MetaObject::invokeFunction(t, [t](){ MetaObject::invokeMethod(t->thread(), t, &QThread::quit); });
    }

    // wait for all threads to be finished
    for (int i = 0; i < threadCount; i++)
    {
        QThread *t = threadList.at(i);
        QSignalSpy spy(t, &QThread::finished);
        if (t->isRunning())
        {
            QVERIFY(spy.count() > 0 || spy.wait(10*taskPerThread*taskDurationMs));
        }
    }

    // check that all tasks have been done
    for (int i = 0; i < taskCount; i++)
    {
        QVERIFY(tasksDone[i]);
    }

    // destroy all threads
    qDeleteAll(threadList);
    threadList.clear();
}


QTEST_GUILESS_MAIN(InvokeOnThreadTest);
