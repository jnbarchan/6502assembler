#ifndef NUMBERBASESPINBOX_H
#define NUMBERBASESPINBOX_H

#include <QSpinBox>

class NumberBaseSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit NumberBaseSpinBox(QWidget *parent = nullptr);

    int fixedDigits() const;
    void setFixedDigits(int newFixedDigits);

signals:

protected:
    virtual QString textFromValue(int val) const override;

private:
    int _fixedDigits;
};

#endif // NUMBERBASESPINBOX_H
