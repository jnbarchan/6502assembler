#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractButton>
#include <QModelIndex>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QMainWindow>

#include "emulator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MemoryViewItemDelegate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Emulator *emulator() const { return g_emulator; }
    ProcessorModel *processorModel() const { return emulator()->processorModel(); }
    Assembler *assembler() const { return emulator()->assembler(); }

    bool haveDoneReset() const;
    void setHaveDoneReset(bool newHaveDoneReset);

    QString currentFileNameToSave() const;
    void setCurrentFileNameToSave(const QString &newCurrentFileNameToSave);

private slots:
    void actionEnablement();
    void sendMessageToConsole(const QString &message);
    void sendCharToConsole(char ch);
    void debugMessage(const QString &message);
    void rbgNumBaseClicked(QAbstractButton *rb);
    void openFile();
    void saveFile();
    void saveFileAs();
    void modelReset();
    void reset();
    void run();
    void stepInto();
    void stepOver();
    void stepOut();
    void codeTextChanged();
    void currentCodeLineNumberChanged(int lineNumber);
    void currentInstructionNumberChanged(int instructionNumber);
    void memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
    void registerChanged(QSpinBox *spn, int value);

private:
    Ui::MainWindow *ui;
    QByteArray codeBytes;
    QTextStream *codeStream;
    bool _haveDoneReset;
    QSpinBox *spnLastChangedColor;
    QString _currentFileNameToSave;
    MemoryViewItemDelegate *tvMemoryViewItemDelegate;
    QModelIndex _lastMemoryModelDataChangedIndex;

    void updateWindowTitle();
    QString scratchFileName() const;
    void openFromFile(QString fileName);
    void saveToFile(QString fileName);
    bool checkSaveFile();
    void scrollToLastMemoryModelDataChangedIndex() const;
    void setRunStopButton(bool run);
    void assembleAndRun(ProcessorModel::RunMode runMode);
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

#endif // MAINWINDOW_H
