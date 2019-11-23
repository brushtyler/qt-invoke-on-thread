#ifndef INVOKEONTHREAD_H
#define INVOKEONTHREAD_H

#include <functional>
#include <QObject>
#include <QString>
#include <QThread>
#include <QtGlobal>

namespace GsPrivate
{

class InvokerBase : public QObject
{
    Q_OBJECT
protected:
    virtual void call_impl() = 0;
public slots:
    void call()
    {
        call_impl();
        this->deleteLater();
    }
};

template <class T>
using rmcvr_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;



template<int ...>
struct seq { };

template<int N, int ...S>
struct gens : gens<N-1, N-1, S...> { };

template<int ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};


template <typename Ret, typename ...Args>
class InvokerFunction : public InvokerBase
{
    using Func = std::function<Ret(Args...)>;

    Func f;
    std::tuple<rmcvr_t<Args>...> a;

public:
    InvokerFunction(Func func, Args ...args)
        : f(func)
        , a(std::make_tuple(rmcvr_t<Args>(args)...))
    {
    }

protected:
    void call_impl()
    {
        call_internal(typename gens<sizeof...(Args)>::type());
    }
    template<int ...S>
    void call_internal(seq<S...>)
    {
        f(std::get<S>(a)...);
    }
};

}

class MetaObject
{
public:
    template <typename Ret, typename ...Args>
    static bool invokeFunction(QThread *thread, std::function<Ret(Args...)> func, Args... args)
    {
        if (QThread::currentThread() != thread)
        {
            GsPrivate::InvokerFunction<Ret,Args...> *invoker = new GsPrivate::InvokerFunction<Ret,Args...>(func, std::forward<Args>(args)...);
            invoker->moveToThread(thread);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            return QMetaObject::invokeMethod(invoker, &GsPrivate::InvokerBase::call, Qt::QueuedConnection);
#else
            return QMetaObject::invokeMethod(invoker, "call", Qt::QueuedConnection);
#endif
        }

        func(args...);
        return true;
    }

    template <typename Ret, typename Obj, typename ...Args>
    static bool invokeMethod(QThread *thread, Obj *obj, Ret(Obj::* func)(Args...), Args... args)
    {
        return invokeFunction(thread, static_cast<std::function<Ret(Obj*, Args...)> >(func), obj, args...);
    }

    template <typename Func, typename ...Args>
    static bool invokeFunction(QThread *thread, Func func, Args... args)
    {
        using ResultType = decltype(func(args...));
        return invokeFunction(thread, static_cast<std::function<ResultType(Args...)> >(func), args...);
    }
};

#endif // INVOKEONTHREAD_H
