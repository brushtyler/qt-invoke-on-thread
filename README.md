# qt-invoke-on-thread
A way to run a function or a method on any QThread (overcome thread affinity and QObject requirement).

### Description

QThreads together with QObject::moveToThread makes thread usage so easy...
But sometimes you need to overcome thread affinity to execute code on a particular thread.
Other times you don't want your class to extend QObject (or QRunnable) nor create your custom MyThread to run code on a particular thread.

This class allow you to do things that usually requires to reimplement QThread::run, wrap your functions or extend QObject.


### Examples

MyClass doesn't extend QObject:

    class MyClass
    {
    private:
        int value;
    public:
        MyClass(int v=0) : value(v) { }
        void setValue(int v) { value = v; }
        int getValue() const { return value; }
    };
        
    MyClass a;
    a.setValue(10);
    
    QThread *thread = new QThread();
    thread->start();

Run MyClass method on thread:

    MetaObject::invokeMethod(thread, &a, &MyClass::setValue, 20); 
    
Run function on thread

    void updateValue(MyClass *b, int newval)
    {
        b->setValue(newval);
    }
    
    MetaObject::invokeFunction(thread, &updateValue, &a, 30);

Run lambda on thread:

    MetaObject::invokeFunction(thread, [](MyClass* b, int newval){ b->setValue(newval); }, &a, 40);
