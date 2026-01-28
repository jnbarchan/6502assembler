#include <QFileDialog>
#include <QMessageBox>
#include <QTextBlock>

#include "emulator.h"
#include "syntaxhighlighter.h"

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#define SAMPLES_RELATIVE_PATH "../../samples"

Emulator *g_emulator;
ProcessorModel *processorModel();


//
// MainWindow Class
//

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    codeStream = nullptr;
    _haveDoneReset = false;
    setCurrentFileNameToSave("");

    g_emulator = new Emulator(this);

    ui->teConsole->installEventFilter(this);

    assembler()->setCodeIncludeDirectories({ SAMPLES_RELATIVE_PATH });
    connect(assembler(), &Assembler::sendMessageToConsole, this, &MainWindow::sendMessageToConsole);
    connect(assembler(), &Assembler::currentCodeLineNumberChanged, this, &MainWindow::currentCodeLineNumberChanged);

    Qt::ConnectionType processorModelConnectionType(Qt::QueuedConnection);
    Qt::ConnectionType queuedChangedSignalsConnectionType(Qt::DirectConnection);

    connect(emulator(), &Emulator::processQueuedChangedSignal, this, &MainWindow::processQueuedChangedSignal);
    connect(emulator(), &Emulator::breakpointChanged, this, &MainWindow::breakpointChanged);

    connect(processorModel(), &ProcessorModel::sendMessageToConsole, this, &MainWindow::sendMessageToConsole, processorModelConnectionType);
    connect(processorModel(), &ProcessorModel::sendStringToConsole, this, &MainWindow::sendStringToConsole, processorModelConnectionType);
    connect(processorModel(), &ProcessorModel::sendCharToConsole, this, &MainWindow::sendCharToConsole, processorModelConnectionType);
    connect(processorModel(), &ProcessorModel::requestCharFromConsole, this, &MainWindow::requestCharFromConsole, processorModelConnectionType);
    connect(processorModel(), &ProcessorModel::endRequestCharFromConsole, this, &MainWindow::endRequestCharFromConsole, processorModelConnectionType);
    connect(processorModel(), &ProcessorModel::statusFlagsChanged, this, [this]() { registerChanged(ui->spnStatusFlags, processorModel()->statusFlags()); }, queuedChangedSignalsConnectionType);
    connect(processorModel(), &ProcessorModel::programCounterChanged, this, [this]() { registerChanged(ui->spnProgramCounter, processorModel()->programCounter()); }, queuedChangedSignalsConnectionType);
    connect(processorModel(), &ProcessorModel::stackRegisterChanged, this, [this]() { registerChanged(ui->spnStackRegister, processorModel()->stackRegister()); }, queuedChangedSignalsConnectionType);
    connect(processorModel(), &ProcessorModel::accumulatorChanged, this, [this]() { registerChanged(ui->spnAccumulator, processorModel()->accumulator()); }, queuedChangedSignalsConnectionType);
    connect(processorModel(), &ProcessorModel::xregisterChanged, this, [this]() { registerChanged(ui->spnXRegister, processorModel()->xregister()); }, queuedChangedSignalsConnectionType);
    connect(processorModel(), &ProcessorModel::yregisterChanged, this, [this]() { registerChanged(ui->spnYRegister, processorModel()->yregister()); }, queuedChangedSignalsConnectionType);
    connect(processorModel(), &ProcessorModel::modelReset, this, &MainWindow::modelReset, processorModelConnectionType);
    modelReset();

    connect(processorModel(), &ProcessorModel::stopRunChanged, this, &MainWindow::actionEnablement, processorModelConnectionType);
    connect(processorModel(), &ProcessorModel::currentInstructionAddressChanged, this, &MainWindow::currentInstructionAddressChanged, queuedChangedSignalsConnectionType);

    ui->codeEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
    connect(ui->codeEditor, &QPlainTextEdit::textChanged, this, &MainWindow::codeTextChanged);
    connect(ui->codeEditor, &QPlainTextEdit::modificationChanged, this, &MainWindow::updateWindowTitle);
    connect(ui->codeEditor, &CodeEditor::lineNumberClicked, this, &MainWindow::codeEditorLineNumberClicked);

    codeEditorLineInfoProvider = new CodeEditorLineInfoProvider(emulator());
    ui->codeEditor->setLineInfoProvider(codeEditorLineInfoProvider);

    syntaxHighlighter = new SyntaxHighlighter(ui->codeEditor->document());

    ui->tvMemory->setModel(processorModel()->memoryModel());
    tvMemoryViewItemDelegate = new MemoryViewItemDelegate(ui->tvMemory);
    ui->tvMemory->setItemDelegate(tvMemoryViewItemDelegate);
    _lastMemoryModelDataChangedIndex = QModelIndex();
    connect(processorModel()->memoryModel(), &MemoryModel::dataChanged, this, &MainWindow::memoryModelDataChanged, queuedChangedSignalsConnectionType);

    watchModel = new WatchModel(processorModel()->memoryModel(), emulator());
    ui->tvWatch->setModel(watchModel);
    tvWatchViewItemDelegate = new WatchViewItemDelegate(ui->tvWatch);
    ui->tvWatch->setItemDelegate(tvWatchViewItemDelegate);

    connect(ui->rbgNumBase, &QButtonGroup::buttonClicked, this, &MainWindow::rbgNumBaseClicked);
    ui->rbNumBaseHex->click();
    ui->spnStatusFlags->setFixedDigits(8);
    ui->spnStatusFlags->setDisplayIntegerBase(2);
    ui->spnProgramCounter->setDisplayIntegerBase(16);
    ui->spnProgramCounter->setFixedDigits(4);
    ui->spnStackRegister->setFixedDigits(2);
    ui->spnStackRegister->setDisplayIntegerBase(16);
    spnLastChangedColor = nullptr;

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actionTurboRun, &QAction::triggered, this, &MainWindow::turboRun);
    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::run);
    connect(ui->actionStepInto, &QAction::triggered, this, &MainWindow::stepInto);
    connect(ui->actionStepOver, &QAction::triggered, this, &MainWindow::stepOver);
    connect(ui->actionStepOut, &QAction::triggered, this, &MainWindow::stepOut);
    connect(ui->actionContinue, &QAction::triggered, this, &MainWindow::continueRun);
    connect(ui->actionReset, &QAction::triggered, this, &MainWindow::reset);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    ui->actionTurboRun->setIcon(ui->btnTurboRun->icon());
    ui->btnTurboRun->setDefaultAction(ui->actionTurboRun);
    ui->actionRun->setIcon(ui->btnRun->icon());
    ui->btnRun->setDefaultAction(ui->actionRun);
    ui->actionStepInto->setIcon(ui->btnStepInto->icon());
    ui->btnStepInto->setDefaultAction(ui->actionStepInto);
    ui->actionStepOver->setIcon(ui->btnStepOver->icon());
    ui->btnStepOver->setDefaultAction(ui->actionStepOver);
    ui->actionStepOut->setIcon(ui->btnStepOut->icon());
    ui->btnStepOut->setDefaultAction(ui->actionStepOut);
    ui->actionContinue->setIcon(ui->btnContinue->icon());
    ui->btnContinue->setDefaultAction(ui->actionContinue);
    ui->actionReset->setIcon(ui->btnReset->icon());
    ui->btnReset->setDefaultAction(ui->actionReset);

    if (QFile::exists(scratchFileName()))
        openFromFile(scratchFileName());
}

MainWindow::~MainWindow()
{
    delete ui;

    delete codeStream;
    delete codeEditorLineInfoProvider;
}

void MainWindow::closeEvent(QCloseEvent *event) /*override*/
{
    saveToFile(scratchFileName());
    if (!checkSaveFile())
    {
        event->ignore();
        return;
    }

    processorModel()->stop();
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->teConsole && event->type() == QEvent::KeyPress)
    {
        auto *ke = static_cast<QKeyEvent *>(event);
        const QString text = ke->text();
        if (!text.isEmpty())
        {
            emit processorModel()->receivedCharFromConsole(text.at(0).toLatin1());
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}


bool MainWindow::haveDoneReset() const
{
    return _haveDoneReset;
}

void MainWindow::setHaveDoneReset(bool newHaveDoneReset)
{
    _haveDoneReset = newHaveDoneReset;
    actionEnablement();
}

QString MainWindow::currentFileNameToSave() const
{
    return _currentFileNameToSave;
}

void MainWindow::setCurrentFileNameToSave(const QString &newCurrentFileNameToSave)
{
    _currentFileNameToSave = newCurrentFileNameToSave;
    ui->actionSave->setEnabled(!_currentFileNameToSave.isEmpty());
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    QString title = "6502 Assembler";
    bool modified = ui->codeEditor->document()->isModified();
    if (!_currentFileNameToSave.isEmpty() || modified)
        title += QString(" (%1%2)").arg(QFileInfo(_currentFileNameToSave).fileName()).arg(modified ? "*" : "");
    setWindowTitle(title);
}


QString MainWindow::scratchFileName() const
{
    return QDir::tempPath() + "/6502assembler_scratchpad.asm";
}

void MainWindow::openFromFile(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        debugMessage(QString("%1: %2").arg(fileName).arg(file.errorString()));
        return;
    }
    ui->codeEditor->setPlainText(file.readAll());
    if (fileName != scratchFileName())
    {
        saveToFile(scratchFileName());
        setCurrentFileNameToSave(fileName);
    }
    reset();
}

void MainWindow::saveToFile(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)
        || file.write(ui->codeEditor->toPlainText().toLatin1()) < 0)
    {
        debugMessage(QString("%1: %2").arg(fileName).arg(file.errorString()));
        return;
    }
    if (fileName != scratchFileName())
    {
        ui->codeEditor->document()->setModified(false);
        setCurrentFileNameToSave(fileName);
    }
}

bool MainWindow::checkSaveFile()
{
    if (!ui->codeEditor->document()->isModified())
        return true;
    if (_currentFileNameToSave.isEmpty() || _currentFileNameToSave == scratchFileName())
        return true;
    const QMessageBox::StandardButton ret =
        QMessageBox::question(
            this,
            "Unsaved Changes",
            "The document has been modified.\nDo you want to save your changes?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save
            );
    switch (ret)
    {
    case QMessageBox::Save:
        saveToFile(_currentFileNameToSave);
        return true;
    case QMessageBox::Discard:
        return true;
    case QMessageBox::Cancel:
    default:
        return false;
    }
}


void MainWindow::scrollToLastMemoryModelDataChangedIndex() const
{
    if (_lastMemoryModelDataChangedIndex.isValid())
        if (!processorModel()->isStackAddress(processorModel()->memoryModel()->indexToAddress(_lastMemoryModelDataChangedIndex)))
            ui->tvMemory->scrollTo(_lastMemoryModelDataChangedIndex, QAbstractItemView::EnsureVisible);
}

void MainWindow::setRunStopButton(bool run)
{
    QAction *action(ui->btnRun->defaultAction());
    action->setText(run ? "Run" : "Stop");
    action->setIcon(QIcon::fromTheme(run ? "media-playback-start" : "media-playback-stop"));
}

void MainWindow::assembleAndRun(ProcessorModel::RunMode runMode)
{
    bool run = runMode == ProcessorModel::Run || runMode == ProcessorModel::TurboRun;
    if (processorModel()->isRunning())
    {
        if (run)
            processorModel()->stop();
        return;
    }
    if (run || !haveDoneReset() || assembler()->needsAssembling())
    {
        saveToFile(scratchFileName());
        reset();
        codeBytes.clear();
        codeBytes.append(ui->codeEditor->toPlainText().toLatin1());
        delete codeStream;
        codeStream = new QTextStream(codeBytes);
        assembler()->setCode(codeStream);

        assembler()->assemble();

        processorModel()->memoryModel()->notifyAllDataChanged();
        watchModel->recalculateAllSymbols();
        if (assembler()->needsAssembling() || runMode == ProcessorModel::NotRunning)
            return;

        processorModel()->setStartNewRun(true);
        processorModel()->setProgramCounter(emulator()->runStartAddress());
    }
    else
        registerChanged(nullptr, 0);

    if (runMode == ProcessorModel::StepInto)
    {
        const Instruction *instruction = processorModel()->nextInstructionToExecute();
        if (instruction != nullptr)
        {
            const InstructionInfo &instructionInfo(instruction->getInstructionInfo());
            if (instructionInfo.operation == Operation::JSR)
            {
                QString filename;
                int lineNumber;
                emulator()->mapInstructionAddressToFileLineNumber(instruction->operand, filename, lineNumber);
                if (!filename.isEmpty())
                    runMode = ProcessorModel::StepOver;
            }
        }
    }

    bool stepOneStatementOnly = runMode == ProcessorModel::StepInto;
    if (!stepOneStatementOnly)
        setRunStopButton(false);

    QCoreApplication::processEvents();
    emulator()->startQueuingChangedSignals();

    switch (runMode)
    {
    case ProcessorModel::NotRunning: break;
    case ProcessorModel::TurboRun: processorModel()->turboRun(); break;
    case ProcessorModel::Run: processorModel()->run(); break;
    case ProcessorModel::StepInto: processorModel()->stepInto(); break;
    case ProcessorModel::StepOver: processorModel()->stepOver(); break;
    case ProcessorModel::StepOut: processorModel()->stepOut(); break;
    case ProcessorModel::Continue: processorModel()->continueRun(); break;
    }

    emulator()->endQueuingChangedSignals();
    QCoreApplication::processEvents();

    if (!stepOneStatementOnly)
        setRunStopButton(true);
}


/*slot*/ void MainWindow::actionEnablement()
{
    bool enable;
    if (!haveDoneReset() || assembler()->needsAssembling())
        enable = true;
    else
        enable = !processorModel()->stopRun();
    ui->btnStepInto->defaultAction()->setEnabled(enable);
    ui->btnStepOver->defaultAction()->setEnabled(enable);
    ui->btnStepOut->defaultAction()->setEnabled(enable);
    ui->btnContinue->defaultAction()->setEnabled(enable);
}

/*slot*/ void MainWindow::debugMessage(const QString &message)
{
    qDebug() << message;
    sendMessageToConsole(message);
}

/*slot*/ void MainWindow::rbgNumBaseClicked(QAbstractButton *rb)
{
    int fixedDigits, integerBase;
    if (rb == ui->rbNumBaseBin)
    {
        fixedDigits = 8;
        integerBase = 2;
    }
    else if (rb == ui->rbNumBaseDec)
    {
        fixedDigits = -1;
        integerBase = 10;
    }
    else if (rb == ui->rbNumBaseHex)
    {
        fixedDigits = 2;
        integerBase = 16;
    }
    else
        return;
    for (NumberBaseSpinBox *spn : { ui->spnAccumulator, ui->spnXRegister, ui->spnYRegister })
    {
        spn->setFixedDigits(fixedDigits);
        spn->setDisplayIntegerBase(integerBase);
    }

    tvMemoryViewItemDelegate->setFixedDigits(fixedDigits);
    tvMemoryViewItemDelegate->setIntegerBase(integerBase);
    int digitSize = ui->tvMemory->font().pointSize() * 9 / 10;
    int digits = fixedDigits > 0 ? fixedDigits : 3;
    ui->tvMemory->horizontalHeader()->setDefaultSectionSize((digits + 1.4) * digitSize);
    QAbstractItemModel *model(ui->tvMemory->model());
    for (int row = 0; row < model->rowCount(); row++)
        for (int col = 0; col < model->columnCount(); col++)
            ui->tvMemory->update(model->index(row, col));
    scrollToLastMemoryModelDataChangedIndex();

    tvWatchViewItemDelegate->setFixedDigits(fixedDigits);
    tvWatchViewItemDelegate->setIntegerBase(integerBase);
    digitSize = ui->tvWatch->font().pointSize() * 9 / 10;
    digits = fixedDigits > 0 ? fixedDigits : 3;
    ui->tvWatch->horizontalHeader()->setDefaultSectionSize((digits + 1.4) * digitSize);
    model = ui->tvWatch->model();
    for (int row = 0; row < model->rowCount(); row++)
        for (int col = 0; col < model->columnCount(); col++)
            ui->tvWatch->update(model->index(row, col));
}

/*slot*/ void MainWindow::codeTextChanged()
{
    // workaround for my https://forum.qt.io/topic/163968/qplaintextedit-qtextedit-signals-textchanged-contentschange-are-emitted-often-when-no-change
    static QString lastText;
    QString text(ui->codeEditor->toPlainText());
    if (text == lastText)
        return;
    lastText = text;

    setHaveDoneReset(false);
    ui->codeEditor->unhighlightCurrentBlock();
    emulator()->clearBreakpoints();
}

/*slot*/ void MainWindow::openFile()
{
    if (!checkSaveFile())
        return;
    QString defaultDir = QDir(SAMPLES_RELATIVE_PATH).exists() ? SAMPLES_RELATIVE_PATH : QString();
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", defaultDir, "*.asm *.inc");
    if (fileName.isEmpty())
        return;
    openFromFile(fileName);
}

/*slot*/ void MainWindow::saveFile()
{
    if (_currentFileNameToSave.isEmpty())
        saveFileAs();
    else
        saveToFile(_currentFileNameToSave);
}

/*slot*/ void MainWindow::saveFileAs()
{
    QString defaultDir = QDir(SAMPLES_RELATIVE_PATH).exists() ? SAMPLES_RELATIVE_PATH : QString();
    QString fileName = QFileDialog::getSaveFileName(this, "Save File As", defaultDir, "*.asm *.inc");
    if (fileName.isEmpty())
        return;
    QFileInfo fileInfo(fileName);
    if (fileInfo.suffix().isEmpty())
        fileName.append(".asm");
    saveToFile(fileName);
}


/*slot*/ void MainWindow::sendMessageToConsole(const QString &message, QBrush colour /*= Qt::transparent*/)
{
    QTextCursor cursor(ui->teConsole->textCursor());
    cursor.movePosition(QTextCursor::End);
    if (cursor.positionInBlock() != 0)
        cursor.insertBlock();
    QTextCharFormat savedFormat(cursor.charFormat());
    if (colour != Qt::transparent)
    {
        QTextCharFormat format;
        format.setForeground(colour);
        cursor.setCharFormat(format);
    }
    cursor.insertText(message);
    if (colour != Qt::transparent)
        cursor.setCharFormat(savedFormat);
    cursor.insertBlock();
    ui->teConsole->setTextCursor(cursor);
}

/*slot*/ void MainWindow::sendStringToConsole(const QString &str)
{
    QTextCursor cursor(ui->teConsole->textCursor());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(str);
    ui->teConsole->setTextCursor(cursor);
}

/*slot*/ void MainWindow::sendCharToConsole(char ch)
{
    QTextCursor cursor(ui->teConsole->textCursor());
    cursor.movePosition(QTextCursor::End);
    if (ch == '\010')  // backspace
        cursor.deletePreviousChar();
    else
        cursor.insertText(QString(ch));
    ui->teConsole->setTextCursor(cursor);
}

/*slot*/ void MainWindow::requestCharFromConsole()
{
    QTextCursor cursor(ui->teConsole->textCursor());
    cursor.movePosition(QTextCursor::End);
    ui->teConsole->setTextCursor(cursor);
    ui->teConsole->setCursorWidth(8);
    ui->teConsole->viewport()->update(); // Force refresh
    ui->teConsole->setFocus();
}

/*slot*/ void MainWindow::endRequestCharFromConsole()
{
    ui->teConsole->setCursorWidth(1);
    ui->teConsole->viewport()->update(); // Force refresh
}

/*slot*/ void MainWindow::modelReset()
{
    ui->spnProgramCounter->setValue(processorModel()->programCounter());
    ui->spnStackRegister->setValue(processorModel()->stackRegister());
    ui->spnAccumulator->setValue(processorModel()->accumulator());
    ui->spnXRegister->setValue(processorModel()->xregister());
    ui->spnYRegister->setValue(processorModel()->yregister());
    ui->spnStatusFlags->setValue(processorModel()->statusFlags());
}

/*slot*/ void MainWindow::reset()
{
    if (processorModel()->isRunning())
        return;
    processorModel()->endRun();
    assembler()->setNeedsAssembling();
    codeBytes.clear();
    delete codeStream;
    codeStream = nullptr;
    currentCodeLineNumberChanged("", -1);
    ui->teConsole->clear();
    _lastMemoryModelDataChangedIndex = QModelIndex();
    registerChanged(nullptr, 0);

    setHaveDoneReset(true);
}

/*slot*/ void MainWindow::turboRun()
{
    assembleAndRun(ProcessorModel::TurboRun);
}

/*slot*/ void MainWindow::run()
{
    assembleAndRun(ProcessorModel::Run);
}

/*slot*/ void MainWindow::continueRun()
{
    assembleAndRun(ProcessorModel::Continue);
}

/*slot*/ void MainWindow::stepInto()
{
    assembleAndRun(ProcessorModel::StepInto);
}

/*slot*/ void MainWindow::stepOver()
{
    assembleAndRun(ProcessorModel::StepOver);
}

/*slot*/ void MainWindow::stepOut()
{
    assembleAndRun(ProcessorModel::StepOut);
}


/*slot*/ void MainWindow::codeEditorLineNumberClicked(int blockNumber)
{
    emulator()->toggleBreakpoint("", blockNumber);
    ui->codeEditor->lineNumberAreaBreakpointUpdated(blockNumber);
}

/*slot*/ void MainWindow::breakpointChanged(int instructionAddress)
{
    if (instructionAddress < 0)
        ui->codeEditor->lineNumberAreaBreakpointUpdated(-1);
    else
    {
        QString filename;
        int lineNumber;
        emulator()->mapInstructionAddressToFileLineNumber(instructionAddress, filename, lineNumber);
        if (filename.isEmpty())
            ui->codeEditor->lineNumberAreaBreakpointUpdated(lineNumber);
    }
}


/*slot*/ void MainWindow::currentCodeLineNumberChanged(const QString &filename, int lineNumber)
{
    if (emulator()->queueChangedSignals())
    {
        emulator()->enqueueQueuedChangedSignal(QueuedChangeSignal::currentCodeLineNumberChanged(filename, lineNumber));
        return;
    }
    if (!filename.isEmpty())
        return;
    if (lineNumber < 0)
    {
        ui->codeEditor->unhighlightCurrentBlock();
        return;
    }
    const QTextDocument *document(ui->codeEditor->document());
    if (lineNumber >= document->blockCount())
        lineNumber = document->blockCount() - 1;
    QTextBlock block(document->findBlockByLineNumber(lineNumber));
    ui->codeEditor->highlightCurrentBlock(block);
}

/*slot*/ void MainWindow::currentInstructionAddressChanged(uint16_t instructionAddress)
{
    if (emulator()->queueChangedSignals())
    {
        emulator()->enqueueQueuedChangedSignal(QueuedChangeSignal::currentInstructionAddressChanged(instructionAddress));
        return;
    }
    QString filename;
    int lineNumber;
    emulator()->mapInstructionAddressToFileLineNumber(instructionAddress, filename, lineNumber);
    if (filename.isEmpty())
        currentCodeLineNumberChanged(filename, lineNumber >= 0 ? lineNumber : ui->codeEditor->blockCount());
}

/*slot*/ void MainWindow::memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles /*= QList<int>()*/)
{
    if (emulator()->queueChangedSignals())
    {
        emulator()->enqueueQueuedChangedSignal(QueuedChangeSignal::memoryModelDataChanged(topLeft, bottomRight, roles));
        return;
    }
    if (roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole) || roles.contains(Qt::ForegroundRole))
        if (topLeft == bottomRight)
        {
            _lastMemoryModelDataChangedIndex = topLeft;
            scrollToLastMemoryModelDataChangedIndex();
        }
}

/*slot*/ void MainWindow::registerChanged(QSpinBox *spn, int value)
{
    if (emulator()->queueChangedSignals())
    {
        emulator()->enqueueQueuedChangedSignal(QueuedChangeSignal::registerChanged(spn, value));
        return;
    }
    bool changeColor = (spn == ui->spnAccumulator || spn == ui->spnXRegister || spn == ui->spnYRegister);
    if (spnLastChangedColor != nullptr && (changeColor || spn == nullptr))
    {
        spnLastChangedColor->setPalette(QPalette());
        spnLastChangedColor = nullptr;
    }
    if (spn != nullptr)
    {
        spn->setValue(value);

        if (changeColor)
        {
            QPalette palette = spn->palette();
            palette.setColor(QPalette::Text, Qt::red);
            spn->setPalette(palette);
            spnLastChangedColor = spn;
        }
    }
}

/*slot*/ void MainWindow::processQueuedChangedSignal(const QueuedChangeSignal &sig)
{
    Q_ASSERT(!emulator()->queueChangedSignals());
    switch (sig.tag)
    {
    case QueuedChangeSignal::CurrentCodeLineNumberChanged:
        currentCodeLineNumberChanged(sig.codeLine.filename, sig.codeLine.lineNumber); break;
    case QueuedChangeSignal::CurrentInstructionAddressChanged:
        currentInstructionAddressChanged(sig.instruction.instructionAddress); break;
    case QueuedChangeSignal::MemoryModelDataChanged:
        memoryModelDataChanged(sig.memory.topLeft, sig.memory.bottomRight, sig.memory.roles);
        watchModel->memoryModelDataChanged(sig.memory.topLeft, sig.memory.bottomRight, sig.memory.roles);
        break;
    case QueuedChangeSignal::RegisterChanged:
        registerChanged(sig._register.spn, sig._register.value); break;
    }
}


//
// CodeEditorLineInfoProvider Class
//

ILineInfoProvider::BreakpointInfo CodeEditorLineInfoProvider::findBreakpointInfo(int blockNumber) const
{
    int instructionAddress = emulator()->findBreakpoint("", blockNumber);
    ILineInfoProvider::BreakpointInfo bpInfo;
    bpInfo.instructionAddress = instructionAddress >= 0 ? instructionAddress : 0;
    return bpInfo;
}
