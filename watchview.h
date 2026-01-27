#ifndef WATCHVIEW_H
#define WATCHVIEW_H

#include "memoryview.h"

//
// WatchView Class
//
class WatchView : public MemoryView
{
    Q_OBJECT
public:
    WatchView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

private slots:
    void resizeSymbolColumn(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());

private:
    QMetaObject::Connection dataChangedConnection;
};


//
// WatchViewItemDelegate Class
//
class WatchViewItemDelegate : public MemoryViewItemDelegate
{
public:
    WatchViewItemDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

#endif // WATCHVIEW_H
