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
    ui->twFlat->sortByColumn(1, Qt::SortOrder::DescendingOrder);
    ui->twFlat->clearContents();
    QAbstractItemModel *model(ui->twFlat->model());
    for (int row = 0; row < _labelHitCounts.count(); row++)
    {
        ui->twFlat->insertRow(row);
        model->setData(model->index(row, 0), _labelHitCounts.at(row).label);
        model->setData(model->index(row, 1), _labelHitCounts.at(row).hitCount);
    }
    ui->twFlat->setSortingEnabled(true);
    ui->twFlat->resizeColumnsToContents();
}

void ProfilingStatisticsWindow::populateTree()
{
    ui->twTree->clear();
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
        //TEMPORARY
        // Bit odd?  My widget locale seems to have OmitGroupSeparator set, preventing 1,000s being shown with their comma
        // QLocale systemLocale = QLocale::system();
        QLocale defaultLocale = QLocale();

        // qDebug() << locale.toString(val) << locale.numberOptions();
        // qDebug() << systemLocale.toString(val) << systemLocale.numberOptions();
        // qDebug() << defaultLocale.toString(val) << defaultLocale.numberOptions();

        return defaultLocale.toString(val);
    }
    return QStyledItemDelegate::displayText(value, locale);
}

void NumberItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const /*override*/
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->displayAlignment = (option->displayAlignment & ~Qt::AlignHorizontal_Mask) | Qt::AlignRight;
}

