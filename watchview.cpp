#include <QHeaderView>
#include <QTimer>

#include "numberbasespinbox.h"

#include "watchview.h"

//
// WatchView Class
//

WatchView::WatchView(QWidget *parent /*= nullptr*/)
    : MemoryView{parent}
{
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    QTimer::singleShot(0, this, [this](){ resizeColumnsToContents(); horizontalHeader()->resizeSection(0, 200); });
}

void WatchView::setModel(QAbstractItemModel *model)
{
    if (dataChangedConnection)
        disconnect(dataChangedConnection);
    MemoryView::setModel(model);
    if (model == nullptr)
        return;
    dataChangedConnection = connect(model, &QAbstractItemModel::dataChanged, this, &WatchView::resizeSymbolColumn);
}

/*slot*/ void WatchView::resizeSymbolColumn(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles /*= QList<int>()*/)
{
    if (topLeft.column() != 0)
        return;
    if (!(roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole)))
        return;
    QFontMetrics fm(font());
    int textWidth = 0;
    for (int row = topLeft.row(); row <= bottomRight.row(); row++)
    {
        QString text = model()->data(topLeft, Qt::DisplayRole).toString();
        textWidth = std::max(textWidth, fm.horizontalAdvance(text) + 12);
    }
    if (textWidth > horizontalHeader()->sectionSize(0))
        horizontalHeader()->resizeSection(0, textWidth);
}


//
// WatchViewItemDelegate Class
//

WatchViewItemDelegate::WatchViewItemDelegate(QObject *parent) : MemoryViewItemDelegate(parent)
{
}

QWidget *WatchViewItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() >= 2)
        return MemoryViewItemDelegate::createEditor(parent, option, index);
    else if (index.column() == 1)
    {
        NumberBaseSpinBox *spn = new NumberBaseSpinBox(parent);
        spn->setFixedDigits(4);
        spn->setDisplayIntegerBase(16);
        spn->setRange(0, 0xffff);
        return spn;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void WatchViewItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const /*override*/
{
    if (index.column() > 0)
        MemoryViewItemDelegate::updateEditorGeometry(editor, option, index);
    else
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
}

void WatchViewItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const /*override*/
{
    QModelIndex indexAddress = index.siblingAtColumn(1);
    QVariant value = indexAddress.data();
    if (!value.isValid() || value.toInt() == 0)
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->text = "";
        return;
    }
    if (index.column() >= 2)
    {
        MemoryViewItemDelegate::initStyleOption(option, index);
        return;
    }
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.column() == 1)
        setOptionTextForNumber(option, index, 4, 16);
}
