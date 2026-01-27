#include <QHeaderView>

#include "memoryview.h"

//
// MemoryView Class
//

MemoryView::MemoryView(QWidget *parent /*= nullptr*/)
    : QTableView{parent}
{
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}


//
// MemoryViewItemDelegate Class
//

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

void MemoryViewItemDelegate::setOptionTextForNumber(QStyleOptionViewItem *option, const QModelIndex &index, int fixedDigits, int integerBase) const
{
    QVariant value(index.data());
    bool ok;
    int val = value.toInt(&ok);
    if (ok)
        option->text = QStringLiteral("%1").arg(val, fixedDigits, integerBase, QChar('0')).toUpper();
}

void MemoryViewItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const /*override*/
{
    QStyledItemDelegate::initStyleOption(option, index);
    setOptionTextForNumber(option, index, _fixedDigits, _integerBase);
}
