#include <QHeaderView>

#include "numberbasespinbox.h"

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

QWidget *MemoryViewItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const /*override*/
{
    NumberBaseSpinBox *spn = new NumberBaseSpinBox(parent);
    spn->setFixedDigits(_fixedDigits);
    spn->setDisplayIntegerBase(_integerBase);
    spn->setRange(0, 0xff);
    QFont font = parent->font();
    int fontSize = font.pointSize();
    if (fontSize > 0)
    {
        font.setPointSize(fontSize + 1);
        spn->setFont(font);
    }
    return spn;
}

void MemoryViewItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const /*override*/
{
    QRect r = option.rect;
    r.adjust(-5, -4, +15, +8);
    editor->setGeometry(r);
}

void MemoryViewItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const /*override*/
{
    QStyledItemDelegate::initStyleOption(option, index);
    setOptionTextForNumber(option, index, _fixedDigits, _integerBase);
}
