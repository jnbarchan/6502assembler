#include <QLineEdit>

#include "numberbasespinbox.h"

//
// NumberBaseSpinBox Class
//

NumberBaseSpinBox::NumberBaseSpinBox(QWidget *parent)
    : QSpinBox{parent}
{
    _fixedDigits = 0;
    _isChr = false;
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
    setAlignment((_fixedDigits > 0 ? Qt::AlignHCenter : Qt::AlignRight) | Qt::AlignVCenter);
    setValue(value());  // force update
}

bool NumberBaseSpinBox::isChr() const
{
    return _isChr;
}

void NumberBaseSpinBox::setIsChr(bool newIsChr)
{
    _isChr = newIsChr;
    setValue(value());  // force update
}

/*virtual*/ QString NumberBaseSpinBox::textFromValue(int val) const /*override*/
{
    QString text;
    if (_isChr)
        text = QLatin1Char(val);
    else
        text = QStringLiteral("%1").arg(val, _fixedDigits, displayIntegerBase(), QChar('0')).toUpper();
    return text;
}

int NumberBaseSpinBox::valueFromText(const QString &text) const
{
    if (_isChr)
        return text.isEmpty() ? 0 : text.at(0).toLatin1();
    else
        return QSpinBox::valueFromText(text);
}
