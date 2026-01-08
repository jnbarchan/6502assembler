#include "memoryview.h"

MemoryView::MemoryView(QWidget *parent /*= nullptr*/)
    : QTableView{parent}
{
}


MemoryViewItemDelegate::MemoryViewItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    _fixedDigits = 2;
    _integerBase = 16;
}

int MemoryViewItemDelegate::fixedDigits() const
{
    return _fixedDigits;
}

void MemoryViewItemDelegate::setFixedDigits(int newFixedDigits)
{
    _fixedDigits = newFixedDigits;
}

int MemoryViewItemDelegate::integerBase() const
{
    return _integerBase;
}

void MemoryViewItemDelegate::setIntegerBase(int newIntegerBase)
{
    _integerBase = newIntegerBase;
}

/*virtual*/ QString MemoryViewItemDelegate::displayText(const QVariant &value, const QLocale &locale) const /*override*/
{
    bool ok;
    int val = value.toInt(&ok);
    if (ok)
        return QStringLiteral("%1").arg(val, _fixedDigits, _integerBase, QChar('0')).toUpper();
    return QStyledItemDelegate::displayText(value, locale);
}
