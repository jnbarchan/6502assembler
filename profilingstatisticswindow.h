#ifndef PROFILINGSTATISTICSWINDOW_H
#define PROFILINGSTATISTICSWINDOW_H

#include <QStyledItemDelegate>
#include <QWidget>

#include "emulator.h"

using ProfilingLabelHitCount = Emulator::ProfilingLabelHitCount;

namespace Ui {
class ProfilingStatisticsWindow;
}

//
// ProfilingStatisticsWindow Class
//
class ProfilingStatisticsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ProfilingStatisticsWindow(QWidget *parent = nullptr);
    ~ProfilingStatisticsWindow();

    void setLabelHitCounts(const QList<ProfilingLabelHitCount> &labelHitCounts);

private:
    Ui::ProfilingStatisticsWindow *ui;

    QList<ProfilingLabelHitCount> _labelHitCounts;

    void populateFlatTable();
    void populateTree();
};


//
// NumberItemDelegate Class
//
class NumberItemDelegate : public QStyledItemDelegate
{
public:
    QString displayText(const QVariant &value, const QLocale &locale) const override;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

#endif // PROFILINGSTATISTICSWINDOW_H
