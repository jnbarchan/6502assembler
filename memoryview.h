#ifndef MEMORYVIEW_H
#define MEMORYVIEW_H

#include <QStyledItemDelegate>
#include <QTableView>

//
// MemoryView Class
//
class MemoryView : public QTableView
{
    Q_OBJECT
public:
    MemoryView(QWidget *parent = nullptr);
};


//
// MemoryViewItemDelegate Class
//
class MemoryViewItemDelegate : public QStyledItemDelegate
{
public:
    MemoryViewItemDelegate(QObject *parent = nullptr);

    int fixedDigits() const;
    void setFixedDigits(int newFixedDigits);

    int integerBase() const;
    void setIntegerBase(int newIntegerBase);

    void setOptionTextForNumber(QStyleOptionViewItem *option, const QModelIndex &index, int fixedDigits, int integerBase) const;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
    int _fixedDigits, _integerBase;
};

#endif // MEMORYVIEW_H
