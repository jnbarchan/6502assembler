#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractButton>
#include <QModelIndex>
#include <QSpinBox>
#include <QMainWindow>

#include "codeeditor.h"
#include "emulator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class FindDialog;
class FindReplaceDialog;

class MemoryViewItemDelegate;
class WatchViewItemDelegate;
class SyntaxHighlighter;
class CodeEditorInfoProvider;

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
    bool eventFilter(QObject *obj, QEvent *event) override;

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
    void showFindDialog();
    void showFindReplaceDialog();
    void sendMessageToConsole(const QString &message, QBrush colour = Qt::transparent);
    void sendStringToConsole(const QString &str);
    void sendCharToConsole(char ch);
    void requestCharFromConsole();
    void endRequestCharFromConsole();
    void modelReset();
    void reset();
    void assembleOnly();
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
    void statusFlagsChanged(int value);

private:
    Ui::MainWindow *ui;
    FindDialog *findDialog;
    FindReplaceDialog *findReplaceDialog;
    QByteArray codeBytes;
    QTextStream *codeStream;
    bool _haveDoneReset;
    QSpinBox *spnLastChangedColor;
    QString _currentFileNameToSave;
    MemoryViewItemDelegate *tvMemoryViewItemDelegate;
    QModelIndex _lastMemoryModelDataChangedIndex;
    WatchViewItemDelegate *tvWatchViewItemDelegate;
    SyntaxHighlighter *syntaxHighlighter;
    CodeEditorInfoProvider *codeEditorInfoProvider;
    WatchModel *watchModel;

    bool _autoAssembleOnFileOpen = true;

    void updateWindowTitle();
    QString scratchFileName() const;
    void openFromFile(QString fileName);
    void saveToFile(QString fileName);
    bool checkSaveFile();
    void showStatusFlagsHeader(uint8_t value, uint8_t prevValue);
    void scrollToLastMemoryModelDataChangedIndex() const;
    void setRunStopButton(bool run);
    void assembleAndRun(ProcessorModel::RunMode runMode);
};


//
// CodeEditorInfoProvider Class
//
class CodeEditorInfoProvider : public ICodeEditorInfoProvider
{
public:
    CodeEditorInfoProvider(const Emulator *emulator) : _emulator(emulator) {}
    ICodeEditorInfoProvider::BreakpointInfo findBreakpointInfo(int blockNumber) const override;
    int findInstructionAddress(int blockNumber) const override;

    QString wordCompletion(const QString& word, int lineNumber) const override;
    QStringListModel *wordCompleterModel(int lineNumber) const override;

private:
    const Emulator *_emulator;
    const Emulator *emulator() const { return _emulator; }
};

#endif // MAINWINDOW_H
