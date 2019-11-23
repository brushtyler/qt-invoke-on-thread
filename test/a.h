#ifndef A_H
#define A_H

#include <QObject>

class A : public QObject
{
    Q_OBJECT

private:
    int m_value;

public:
    A(int value=0) : m_value(value) {}

public slots:
    int getValue() const { return m_value; }
    void setValue(int value) { m_value = value;  emit valueChanged(value); }

signals:
    void valueChanged(int value);

};

#endif // A_H
