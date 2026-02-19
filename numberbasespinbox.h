#ifndef NUMBERBASESPINBOX_H
#define NUMBERBASESPINBOX_H

#include <QSpinBox>

//
// NumberBaseSpinBox Class
//
class NumberBaseSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit NumberBaseSpinBox(QWidget *parent = nullptr);

    int fixedDigits() const;
    void setFixedDigits(int newFixedDigits);
    bool isChr() const;
    void setIsChr(bool newIsChr);

signals:

protected:
    virtual QString textFromValue(int val) const override;
    int valueFromText(const QString &text) const override;

private:
    int _fixedDigits;
    bool _isChr;
};

#endif // NUMBERBASESPINBOX_H
