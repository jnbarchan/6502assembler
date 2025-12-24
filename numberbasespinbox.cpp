#include "numberbasespinbox.h"

NumberBaseSpinBox::NumberBaseSpinBox(QWidget *parent)
    : QSpinBox{parent}
{
    _fixedDigits = 0;
}

int NumberBaseSpinBox::fixedDigits() const
{
    return _fixedDigits;
}

void NumberBaseSpinBox::setFixedDigits(int newFixedDigits)
{
    _fixedDigits = newFixedDigits;
    int digitSize = font().pointSize() * 9 / 10;
    int digits = _fixedDigits > 0 ? _fixedDigits : 3;
    setMinimumWidth((digits + 2) * digitSize);
}

/*virtual*/ QString NumberBaseSpinBox::textFromValue(int val) const /*override*/
{
    QString text = QStringLiteral("%1").arg(val, _fixedDigits, displayIntegerBase(), QChar('0'));
    return text;
}
