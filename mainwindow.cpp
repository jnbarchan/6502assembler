#include <QFileDialog>
#include <QMessageBox>
#include <QTextBlock>

#include "processormodel.h"

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#define SAMPLES_RELATIVE_PATH "../../samples"

ProcessorModel *g_processorModel;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    codeStream = nullptr;
    _haveDoneReset = false;
    setCurrentFileNameToSave("");

    g_processorModel = new ProcessorModel(this);
    connect(g_processorModel, &ProcessorModel::sendMessageToConsole, this, &MainWindow::sendMessageToConsole);
    connect(g_processorModel, &ProcessorModel::sendCharToConsole, this, &MainWindow::sendCharToConsole);
    connect(g_processorModel, &ProcessorModel::statusFlagsChanged, this, [this]() { registerChanged(ui->spnStatusFlags, g_processorModel->statusFlags()); });
    connect(g_processorModel, &ProcessorModel::stackRegisterChanged, this, [this]() { registerChanged(ui->spnStackRegister, g_processorModel->stackRegister()); });
    connect(g_processorModel, &ProcessorModel::accumulatorChanged, this, [this]() { registerChanged(ui->spnAccumulator, g_processorModel->accumulator()); });
    connect(g_processorModel, &ProcessorModel::xregisterChanged, this, [this]() { registerChanged(ui->spnXRegister, g_processorModel->xregister()); });
    connect(g_processorModel, &ProcessorModel::yregisterChanged, this, [this]() { registerChanged(ui->spnYRegister, g_processorModel->yregister()); });
    connect(g_processorModel, &ProcessorModel::modelReset, this, &MainWindow::modelReset);
    modelReset();

    connect(g_processorModel, &ProcessorModel::stopRunChanged, this, &MainWindow::actionEnablement);

    ui->codeEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
    connect(g_processorModel, &ProcessorModel::currentCodeLineNumberChanged, this, &MainWindow::currentCodeLineNumberChanged);
    connect(ui->codeEditor, &QPlainTextEdit::textChanged, this, &MainWindow::codeTextChanged);
    connect(ui->codeEditor, &QPlainTextEdit::modificationChanged, this, &MainWindow::updateWindowTitle);

    ui->tvMemory->setModel(g_processorModel->memoryModel());
    ui->tvMemory->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tvMemory->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    tvMemoryViewItemDelegate = new MemoryViewItemDelegate(ui->tvMemory);
    ui->tvMemory->setItemDelegate(tvMemoryViewItemDelegate);
    _lastMemoryModelDataChangedIndex = QModelIndex();
    connect(g_processorModel->memoryModel(), &MemoryModel::dataChanged, this, &MainWindow::memoryModelDataChanged);

    connect(ui->rbgNumBase, &QButtonGroup::buttonClicked, this, &MainWindow::rbgNumBaseClicked);
    ui->rbNumBaseHex->click();
    ui->spnStatusFlags->setFixedDigits(8);
    ui->spnStatusFlags->setDisplayIntegerBase(2);
    ui->spnStackRegister->setFixedDigits(2);
    ui->spnStackRegister->setDisplayIntegerBase(16);
    spnLastChangedColor = nullptr;

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::run);
    connect(ui->actionStepInto, &QAction::triggered, this, &MainWindow::stepInto);
    connect(ui->actionStepOver, &QAction::triggered, this, &MainWindow::stepOver);
    connect(ui->actionStepOut, &QAction::triggered, this, &MainWindow::stepOut);
    connect(ui->actionReset, &QAction::triggered, this, &MainWindow::reset);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    ui->actionRun->setIcon(ui->btnRun->icon());
    ui->btnRun->setDefaultAction(ui->actionRun);
    ui->actionStepInto->setIcon(ui->btnStepInto->icon());
    ui->btnStepInto->setDefaultAction(ui->actionStepInto);
    ui->actionStepOver->setIcon(ui->btnStepOver->icon());
    ui->btnStepOver->setDefaultAction(ui->actionStepOver);
    ui->actionStepOut->setIcon(ui->btnStepOut->icon());
    ui->btnStepOut->setDefaultAction(ui->actionStepOut);
    ui->actionReset->setIcon(ui->btnReset->icon());
    ui->btnReset->setDefaultAction(ui->actionReset);

    if (QFile::exists(scratchFileName()))
        openFromFile(scratchFileName());
}

MainWindow::~MainWindow()
{
    delete ui;

    delete codeStream;
}

void MainWindow::closeEvent(QCloseEvent *event) /*override*/
{
    saveToFile(scratchFileName());
    if (!checkSaveFile())
        event->ignore();
    else
        QMainWindow::closeEvent(event);
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
        || file.write(ui->codeEditor->toPlainText().toUtf8()) < 0)
    {
        debugMessage(QString("%1: %2").arg(fileName).arg(file.errorString()));
        return;
    }
    if (fileName != scratchFileName())
        setCurrentFileNameToSave(fileName);
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
        if (!g_processorModel->isStackAddress(g_processorModel->memoryModel()->indexToAddress(_lastMemoryModelDataChangedIndex)))
            ui->tvMemory->scrollTo(_lastMemoryModelDataChangedIndex, QAbstractItemView::EnsureVisible);
}

void MainWindow::setRunStopButton(bool run)
{
    QAction *action(ui->btnRun->defaultAction());
    action->setText(run ? "Run" : "Stop");
    action->setIcon(QIcon::fromTheme(run ? "media-playback-start" : "media-playback-stop"));
}


/*slot*/ void MainWindow::actionEnablement()
{
    bool enable;
    if (!haveDoneReset())
        enable = true;
    else
        enable = !g_processorModel->stopRun();
    ui->btnStepInto->defaultAction()->setEnabled(enable);
    ui->btnStepOver->defaultAction()->setEnabled(enable);
    ui->btnStepOut->defaultAction()->setEnabled(enable);
}


/*slot*/ void MainWindow::sendMessageToConsole(const QString &message)
{
    ui->teConsole->appendPlainText(message);
}

/*slot*/ void MainWindow::sendCharToConsole(char ch)
{
    ui->teConsole->insertPlainText(QString(ch));
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
}

/*slot*/ void MainWindow::openFile()
{
    if (!checkSaveFile())
        return;
    QString defaultDir = QDir(SAMPLES_RELATIVE_PATH).exists() ? SAMPLES_RELATIVE_PATH : QString();
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", defaultDir, "*.asm");
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
    QString fileName = QFileDialog::getSaveFileName(this, "Save File As", defaultDir, "*.asm");
    if (fileName.isEmpty())
        return;
    QFileInfo fileInfo(fileName);
    if (fileInfo.suffix().isEmpty())
        fileName.append(".asm");
    saveToFile(fileName);
}

/*slot*/ void MainWindow::modelReset()
{
    ui->spnStackRegister->setValue(g_processorModel->stackRegister());
    ui->spnAccumulator->setValue(g_processorModel->accumulator());
    ui->spnXRegister->setValue(g_processorModel->xregister());
    ui->spnYRegister->setValue(g_processorModel->yregister());
    ui->spnStatusFlags->setValue(g_processorModel->statusFlags());
}

/*slot*/ void MainWindow::reset()
{
    if (g_processorModel->isRunning())
        return;
    codeBytes.clear();
    codeBytes.append(ui->codeEditor->toPlainText().toUtf8());
    delete codeStream;
    codeStream = new QTextStream(codeBytes);
    g_processorModel->setCode(codeStream);
    g_processorModel->restart();
    currentCodeLineNumberChanged(-1);
    registerChanged(nullptr, 0);
    ui->teConsole->clear();
    _lastMemoryModelDataChangedIndex = QModelIndex();
    setHaveDoneReset(true);
}

/*slot*/ void MainWindow::run()
{
    if (g_processorModel->isRunning())
    {
        g_processorModel->stop();
        return;
    }
    saveToFile(scratchFileName());
    reset();

    setRunStopButton(false);
    g_processorModel->run(false, false);
    setRunStopButton(true);
}

/*slot*/ void MainWindow::stepInto()
{
    if (g_processorModel->isRunning())
        return;
    if (!haveDoneReset())
    {
        saveToFile(scratchFileName());
        reset();
    }
    registerChanged(nullptr, 0);
    g_processorModel->step();
}

/*slot*/ void MainWindow::stepOver()
{
    if (g_processorModel->isRunning())
        return;
    if (!haveDoneReset())
    {
        saveToFile(scratchFileName());
        reset();
    }
    registerChanged(nullptr, 0);

    setRunStopButton(false);
    g_processorModel->run(true, false);
    setRunStopButton(true);
}

/*slot*/ void MainWindow::stepOut()
{
    if (g_processorModel->isRunning())
        return;
    if (!haveDoneReset())
    {
        saveToFile(scratchFileName());
        reset();
    }
    registerChanged(nullptr, 0);

    setRunStopButton(false);
    g_processorModel->run(false, true);
    setRunStopButton(true);
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
    if (!ui->codeEditor->extraSelections().isEmpty())
        ui->codeEditor->setExtraSelections({});
}

/*slot*/ void MainWindow::currentCodeLineNumberChanged(int lineNumber)
{
    if (lineNumber < 0)
    {
        if (!ui->codeEditor->extraSelections().isEmpty())
            ui->codeEditor->setExtraSelections({});
        return;
    }
    QTextBlock block(ui->codeEditor->document()->findBlockByLineNumber(lineNumber));
    QTextCursor cursor(block);
    QTextEdit::ExtraSelection selection;
    cursor.movePosition(QTextCursor::StartOfLine);
    selection.cursor = cursor;
    selection.format.setBackground(QColor(255, 220, 180)); // light orange
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    ui->codeEditor->setExtraSelections({ selection });
    ui->codeEditor->setTextCursor(cursor);
    ui->codeEditor->centerCursor();
}

/*slot*/ void MainWindow::memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    if (roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole) || roles.contains(Qt::ForegroundRole))
        if (topLeft == bottomRight)
        {
            _lastMemoryModelDataChangedIndex = topLeft;
            scrollToLastMemoryModelDataChangedIndex();
        }
}

/*slot*/ void MainWindow::registerChanged(QSpinBox *spn, int value)
{
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


MemoryViewItemDelegate::MemoryViewItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    _fixedDigits = 2;
    _integerBase = 16;
}

int MemoryViewItemDelegate::fixedDigits() const
{
    return _fixedDigits;
}

void MemoryViewItemDelegate::setFixedDigits(int newFixedDigits)
{
    _fixedDigits = newFixedDigits;
}

int MemoryViewItemDelegate::integerBase() const
{
    return _integerBase;
}

void MemoryViewItemDelegate::setIntegerBase(int newIntegerBase)
{
    _integerBase = newIntegerBase;
}

/*virtual*/ QString MemoryViewItemDelegate::displayText(const QVariant &value, const QLocale &locale) const /*override*/
{
    bool ok;
    int val = value.toInt(&ok);
    if (ok)
        return QStringLiteral("%1").arg(val, _fixedDigits, _integerBase, QChar('0')).toUpper();
    return QStyledItemDelegate::displayText(value, locale);
}

