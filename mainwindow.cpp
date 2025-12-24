#include <QFileDialog>
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

    connect(ui->rbgNumBase, &QButtonGroup::buttonClicked, this, &MainWindow::rbgNumBaseClicked);
    ui->rbNumBaseHex->click();
    ui->spnStatusFlags->setFixedDigits(8);
    ui->spnStatusFlags->setDisplayIntegerBase(2);
    ui->spnStackRegister->setFixedDigits(2);
    ui->spnStackRegister->setDisplayIntegerBase(16);

    codeStream = nullptr;
    _haveDoneReset = false;

    g_processorModel = new ProcessorModel(this);
    connect(g_processorModel, &ProcessorModel::sendMessageToConsole, this, &MainWindow::sendMessageToConsole);
    connect(g_processorModel, &ProcessorModel::sendCharToConsole, this, &MainWindow::sendCharToConsole);
    connect(g_processorModel, &ProcessorModel::statusFlagsChanged, this, [this]() { ui->spnStatusFlags->setValue(g_processorModel->statusFlags()); });
    connect(g_processorModel, &ProcessorModel::stackRegisterChanged, this, [this]() { ui->spnStackRegister->setValue(g_processorModel->stackRegister()); });
    connect(g_processorModel, &ProcessorModel::accumulatorChanged, this, [this]() { ui->spnAccumulator->setValue(g_processorModel->accumulator()); });
    connect(g_processorModel, &ProcessorModel::xregisterChanged, this, [this]() { ui->spnXRegister->setValue(g_processorModel->xregister()); });
    connect(g_processorModel, &ProcessorModel::yregisterChanged, this, [this]() { ui->spnYRegister->setValue(g_processorModel->yregister()); });
    connect(g_processorModel, &ProcessorModel::modelReset, this, &MainWindow::modelReset);
    modelReset();

    connect(g_processorModel, &ProcessorModel::stopRunChanged, this, &MainWindow::actionEnablement);

    ui->codeEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
    connect(g_processorModel, &ProcessorModel::currentCodeLineNumberChanged, this, &MainWindow::currentCodeLineNumberChanged);
    connect(ui->codeEditor, &QPlainTextEdit::textChanged, this, &MainWindow::codeTextChanged);

    ui->tvMemory->setModel(g_processorModel->memoryModel());
    ui->tvMemory->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tvMemory->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tvMemory->setItemDelegate(new MemoryViewItemDelegate(ui->tvMemory));
    connect(g_processorModel->memoryModel(), &MemoryModel::dataChanged, this, &MainWindow::memoryModelDataChanged);

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::run);
    connect(ui->actionStep, &QAction::triggered, this, &MainWindow::step);
    connect(ui->actionReset, &QAction::triggered, this, &MainWindow::reset);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    ui->actionRun->setIcon(ui->btnRun->icon());
    ui->btnRun->setDefaultAction(ui->actionRun);
    ui->actionStep->setIcon(ui->btnStep->icon());
    ui->btnStep->setDefaultAction(ui->actionStep);
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


bool MainWindow::haveDoneReset() const
{
    return _haveDoneReset;
}

void MainWindow::setHaveDoneReset(bool newHaveDoneReset)
{
    _haveDoneReset = newHaveDoneReset;
    actionEnablement();
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
}


/*slot*/ void MainWindow::actionEnablement()
{
    bool enable;
    if (!haveDoneReset())
        enable = true;
    else
        enable = !g_processorModel->stopRun();
    ui->btnStep->defaultAction()->setEnabled(enable);
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
    for (NumberBaseSpinBox *spn : { ui->spnAccumulator, ui->spnXRegister, ui->spnYRegister })
        if (rb == ui->rbNumBaseBin)
        {
            spn->setFixedDigits(8);
            spn->setDisplayIntegerBase(2);
        }
        else if (rb == ui->rbNumBaseDec)
        {
            spn->setFixedDigits(-1);
            spn->setDisplayIntegerBase(10);
        }
        else if (rb == ui->rbNumBaseHex)
        {
            spn->setFixedDigits(2);
            spn->setDisplayIntegerBase(16);
        }
}

/*slot*/ void MainWindow::openFile()
{
    QString defaultDir = QDir(SAMPLES_RELATIVE_PATH).exists() ? SAMPLES_RELATIVE_PATH : QString();
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", defaultDir, "*.asm");
    if (fileName.isEmpty())
        return;
    openFromFile(fileName);
}

/*slot*/ void MainWindow::saveFile()
{
    QString defaultDir = QDir(SAMPLES_RELATIVE_PATH).exists() ? SAMPLES_RELATIVE_PATH : QString();
    QString fileName = QFileDialog::getSaveFileName(this, "Save File", defaultDir, "*.asm");
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
    ui->teConsole->clear();
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
    ui->btnRun->setText("Stop");

    g_processorModel->run();

    ui->btnRun->setText("Run");
}

/*slot*/ void MainWindow::step()
{
    if (g_processorModel->isRunning())
        return;
    if (!haveDoneReset())
    {
        saveToFile(scratchFileName());
        reset();
    }
    g_processorModel->step();
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
    // const QTextCursor savedCursor(ui->codeEditor->textCursor());
    QTextEdit::ExtraSelection selection;
    cursor.movePosition(QTextCursor::StartOfLine);
    selection.cursor = cursor;
    selection.format.setBackground(QColor(255, 220, 180)); // light orange
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    ui->codeEditor->setExtraSelections({ selection });

    ui->codeEditor->setTextCursor(cursor);
    ui->codeEditor->centerCursor();
    // ui->codeEditor->ensureCursorVisible();
    // ui->codeEditor->setTextCursor(savedCursor);
}

/*slot*/ void MainWindow::memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    if (roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole))
    if (topLeft == bottomRight)
        ui->tvMemory->scrollTo(topLeft, QAbstractItemView::EnsureVisible);
}


MemoryViewItemDelegate::MemoryViewItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

/*virtual*/ QString MemoryViewItemDelegate::displayText(const QVariant &value, const QLocale &locale) const /*override*/
{
    bool ok;
    int val = value.toInt(&ok);
    if (ok)
        return QStringLiteral("%1").arg(val, 2, 16, QChar('0'));
    return QStyledItemDelegate::displayText(value, locale);
}
