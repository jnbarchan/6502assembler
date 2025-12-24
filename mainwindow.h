#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractButton>
#include <QStyledItemDelegate>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool haveDoneReset() const;
    void setHaveDoneReset(bool newHaveDoneReset);

private slots:
    void sendMessageToConsole(const QString &message);
    void sendCharToConsole(char ch);
    void debugMessage(const QString &message);
    void rbgNumBaseClicked(QAbstractButton *rb);
    void openFile();
    void saveFile();
    void modelReset();
    void reset();
    void run();
    void step();
    void codeTextChanged();
    void currentCodeLineNumberChanged(int lineNumber);
    void memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());

private:
    Ui::MainWindow *ui;
    QByteArray codeBytes;
    QTextStream *codeStream;
    bool _haveDoneReset;

    QString scratchFileName() const;
    void openFromFile(QString fileName);
    void saveToFile(QString fileName);
};


class MemoryViewItemDelegate : public QStyledItemDelegate
{
public:
    MemoryViewItemDelegate(QObject *parent = nullptr);

    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;
};

#endif // MAINWINDOW_H
