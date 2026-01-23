#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractButton>
#include <QModelIndex>
#include <QSpinBox>
#include <QMainWindow>

#include "codeeditor.h"
#include "emulator.h"

using QueuedChangeSignal = Emulator::QueuedChangeSignal;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MemoryViewItemDelegate;
class SyntaxHighlighter;
class CodeEditorLineInfoProvider;

//
// MainWindow Class
//
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
    void debugMessage(const QString &message);
    void rbgNumBaseClicked(QAbstractButton *rb);
    void codeTextChanged();
    void openFile();
    void saveFile();
    void saveFileAs();
    void sendMessageToConsole(const QString &message, QBrush colour = Qt::transparent);
    void sendCharToConsole(char ch);
    void modelReset();
    void reset();
    void turboRun();
    void run();
    void continueRun();
    void stepInto();
    void stepOver();
    void stepOut();
    void codeEditorLineNumberClicked(int blockNumber);
    void breakpointChanged(int instructionAddress);
    void currentCodeLineNumberChanged(const QString &filename, int lineNumber);
    void currentInstructionAddressChanged(uint16_t instructionAddress);
    void memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
    void registerChanged(QSpinBox *spn, int value);
    void processQueuedChangedSignal(const QueuedChangeSignal &sig);

private:
    Ui::MainWindow *ui;
    QByteArray codeBytes;
    QTextStream *codeStream;
    bool _haveDoneReset;
    QSpinBox *spnLastChangedColor;
    QString _currentFileNameToSave;
    MemoryViewItemDelegate *tvMemoryViewItemDelegate;
    QModelIndex _lastMemoryModelDataChangedIndex;
    SyntaxHighlighter *syntaxHighlighter;
    CodeEditorLineInfoProvider *codeEditorLineInfoProvider;

    void updateWindowTitle();
    QString scratchFileName() const;
    void openFromFile(QString fileName);
    void saveToFile(QString fileName);
    bool checkSaveFile();
    void scrollToLastMemoryModelDataChangedIndex() const;
    void setRunStopButton(bool run);
    void assembleAndRun(ProcessorModel::RunMode runMode);
};


//
// CodeEditorLineInfoProvider Class
//
class CodeEditorLineInfoProvider : public ILineInfoProvider
{
public:
    CodeEditorLineInfoProvider(const Emulator *emulator) : _emulator(emulator) {}
    ILineInfoProvider::BreakpointInfo findBreakpointInfo(int blockNumber) const override;

private:
    const Emulator *_emulator;
    const Emulator *emulator() const { return _emulator; }
};

#endif // MAINWINDOW_H
