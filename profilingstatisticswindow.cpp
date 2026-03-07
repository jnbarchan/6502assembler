#include "profilingstatisticswindow.h"
#include "ui_profilingstatisticswindow.h"

//
// ProfilingStatisticsWindow Class
//

ProfilingStatisticsWindow::ProfilingStatisticsWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProfilingStatisticsWindow)
{
    setWindowFlag(Qt::Window);
    ui->setupUi(this);

    ui->twFlat->setItemDelegateForColumn(1, new NumberItemDelegate);
    ui->twFlat->setItemDelegateForColumn(2, new NumberItemDelegate);
    ui->twTree->setItemDelegateForColumn(1, new NumberItemDelegate);
    ui->twTree->setItemDelegateForColumn(2, new NumberItemDelegate);
    ui->twTree->setItemDelegateForColumn(3, new NumberItemDelegate);
    ui->twTree->setItemDelegateForColumn(4, new NumberItemDelegate);

    _labelHitCounts.clear();
    ui->twFlat->clearContents();
    ui->twTree->clear();
}

ProfilingStatisticsWindow::~ProfilingStatisticsWindow()
{
    delete ui;
}

void ProfilingStatisticsWindow::setLabelHitCounts(const QList<ProfilingLabelHitCount> &labelHitCounts)
{
    _labelHitCounts = labelHitCounts;
    populateFlatTable();
    populateTree();
}

void ProfilingStatisticsWindow::populateFlatTable()
{
    ui->twFlat->setSortingEnabled(false);
    ui->twFlat->sortByColumn(2, Qt::SortOrder::DescendingOrder);
    ui->twFlat->clearContents();

    QAbstractItemModel *model(ui->twFlat->model());
    for (int row = 0; row < _labelHitCounts.count(); row++)
    {
        ui->twFlat->insertRow(row);
        const QString &label(_labelHitCounts.at(row).label);
        model->setData(model->index(row, 0), label);
        QColor qColor(label.indexOf('.') > 0 ? Qt::darkMagenta : Qt::magenta);
        model->setData(model->index(row, 0), qColor, Qt::ForegroundRole);
        model->setData(model->index(row, 1), _labelHitCounts.at(row).hitCount);
        model->setData(model->index(row, 2), _labelHitCounts.at(row).cycleCount);
    }

    ui->twFlat->setSortingEnabled(true);
    ui->twFlat->resizeColumnsToContents();
}

void ProfilingStatisticsWindow::populateTree()
{
    ui->twTree->setSortingEnabled(false);
    ui->twTree->sortByColumn(4, Qt::SortOrder::DescendingOrder);
    ui->twTree->clear();

    for (int row = 0; row < _labelHitCounts.count(); row++)
    {
        QString scopeLabel, subLabel;
        scopeLabel = _labelHitCounts.at(row).label;
        int split = scopeLabel.indexOf('.');
        if (split > 0)
        {
            subLabel = scopeLabel.mid(split);
            scopeLabel = scopeLabel.left(split);
        }
        Q_ASSERT(!scopeLabel.isEmpty());

        QTreeWidgetItem *topLevelItem = nullptr;
        for (int i = 0; i < ui->twTree->topLevelItemCount() && topLevelItem == nullptr; i++)
            if (ui->twTree->topLevelItem(i)->text(0) == scopeLabel)
                topLevelItem = ui->twTree->topLevelItem(i);
        if (topLevelItem == nullptr)
        {
            topLevelItem = new QTreeWidgetItem(ui->twTree);
            topLevelItem->setText(0, scopeLabel);
            topLevelItem->setForeground(0, Qt::magenta);
            topLevelItem->setData(1, Qt::EditRole, 0);
            topLevelItem->setData(2, Qt::EditRole, 0);
        }
        if (subLabel.isEmpty())
        {
            topLevelItem->setData(1, Qt::EditRole, _labelHitCounts.at(row).hitCount);
            topLevelItem->setData(2, Qt::EditRole, _labelHitCounts.at(row).cycleCount);
        }
        else
        {
            QTreeWidgetItem *childItem = new QTreeWidgetItem(topLevelItem);
            childItem->setText(0, subLabel);
            childItem->setForeground(0, Qt::darkMagenta);
            childItem->setData(1, Qt::EditRole, _labelHitCounts.at(row).hitCount);
            childItem->setData(2, Qt::EditRole, _labelHitCounts.at(row).cycleCount);
        }
    }
    for (int i = 0; i < ui->twTree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *topLevelItem = ui->twTree->topLevelItem(i);
        int totalHits = topLevelItem->data(1, Qt::EditRole).toInt();
        int totalCycles = topLevelItem->data(2, Qt::EditRole).toInt();
        for (int j = 0; j < topLevelItem->childCount(); j++)
        {
            QTreeWidgetItem *childItem = topLevelItem->child(j);
            int hits = childItem->data(1, Qt::EditRole).toInt();
            childItem->setData(3, Qt::EditRole, hits);
            totalHits += hits;
            int cycles = childItem->data(2, Qt::EditRole).toInt();
            childItem->setData(4, Qt::EditRole, cycles);
            totalCycles += cycles;
        }
        topLevelItem->setData(3, Qt::EditRole, totalHits);
        topLevelItem->setData(4, Qt::EditRole, totalCycles);
    }

    ui->twTree->setSortingEnabled(true);
    ui->twTree->collapseAll();
    for (int i = 0; i < ui->twTree->columnCount(); i++)
        ui->twTree->resizeColumnToContents(i);
}


//
// NumberItemDelegate Class
//

QString NumberItemDelegate::displayText(const QVariant &value, const QLocale &locale) const /*override*/
{
    bool ok;
    int val = value.toInt(&ok);
    if (ok)
    {
        // locale.numberOptions() has QLocale::OmitGroupSeparator set
        // void QAbstractItemView::initViewItemOption(QStyleOptionViewItem *option) const explicitly sets:
        //     option->locale = locale();
        //     option->locale.setNumberOptions(QLocale::OmitGroupSeparator);
        QLocale defaultLocale = QLocale();
        return defaultLocale.toString(val);
    }
    return QStyledItemDelegate::displayText(value, locale);
}

void NumberItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const /*override*/
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->displayAlignment = (option->displayAlignment & ~Qt::AlignHorizontal_Mask) | Qt::AlignRight;
}

