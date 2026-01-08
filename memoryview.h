#ifndef MEMORYVIEW_H
#define MEMORYVIEW_H

#include <QStyledItemDelegate>
#include <QTableView>

class MemoryView : public QTableView
{
    Q_OBJECT
public:
    MemoryView(QWidget *parent = nullptr);
};


class MemoryViewItemDelegate : public QStyledItemDelegate
{
public:
    MemoryViewItemDelegate(QObject *parent = nullptr);

    int fixedDigits() const;
    void setFixedDigits(int newFixedDigits);

    int integerBase() const;
    void setIntegerBase(int newIntegerBase);

    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;

private:
    int _fixedDigits, _integerBase;
};

#endif // MEMORYVIEW_H
