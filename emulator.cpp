#include <QMimeData>

#include "appsettings.h"
#include "emulator.h"

using CodeFileLineNumber = Assembler::CodeFileLineNumber;
using CodeLabels = Assembler::CodeLabels;
using ScopeLabel = Assembler::ScopeLabel;
using MacroDefinitions = Assembler::MacroDefinitions;
using Profiling = ProcessorModel::Profiling;

//
// Emulator Class
//
Emulator::Emulator(QObject *parent)
    : QObject{parent}
{
    Assembly::initInstructionInfo();

    _processorModel = new ProcessorModel(this);
    _memory = _processorModel->memory();
    _instructions = _processorModel->instructions();
    processorBreakpointProvider = new ProcessorBreakpointProvider(this);
    _processorModel->setProcessorBreakpointProvider(processorBreakpointProvider);

    _assembler = new Assembler(this);
    _assembler->setMemory(_memory);
    _assembler->setInstructions(_instructions);
    assemblerBreakpointProvider = new AssemblerBreakpointProvider(this);
    _assembler->setAssemblerBreakpointProvider(assemblerBreakpointProvider);

    _wordCompleterModel = new QStringListModel(this);
}

Emulator::~Emulator()
{
    delete processorBreakpointProvider;
    delete assemblerBreakpointProvider;
}

const uint16_t Emulator::runStartAddress() const
{
    for (const QString &label : { "reset", "start", "START", "main", "MAIN", })
    {
        Assembler::ExpressionValue value(assembler()->codeLabelValue(label));
        if (value.isValid() && value.intValue > 0)
            return value.intValue;
    }
    return assembler()->defaultLocationCounter();
}


void Emulator::mapInstructionAddressToFileLineNumber(uint16_t instructionAddress, QString &filename, int &lineNumber) const
{
    filename.clear();
    lineNumber = -1;
    const QList<CodeFileLineNumber> &codeFileLineNumbers(_assembler->instructionsCodeFileLineNumbers());
    const CodeFileLineNumber cfln(instructionAddress, "", 0);
    auto it = std::lower_bound(codeFileLineNumbers.begin(), codeFileLineNumbers.end(), cfln,
                               [](const CodeFileLineNumber &lhs, const CodeFileLineNumber &rhs) -> bool { return lhs._locationCounter < rhs._locationCounter; });
    if (it != codeFileLineNumbers.end())
    {
        filename = it->_codeFilename;
        lineNumber = it->_currentCodeLineNumber;
        return;
    }
}

uint16_t Emulator::mapFileLineNumberToInstructionAddress(const QString &filename, int lineNumber, bool exact /*= false*/) const
{
    const QList<CodeFileLineNumber> &codeFileLineNumbers(_assembler->instructionsCodeFileLineNumbers());
    for (const CodeFileLineNumber &cfln : codeFileLineNumbers)
        if (cfln._codeFilename == filename)
            if (exact ? cfln._currentCodeLineNumber == lineNumber : cfln._currentCodeLineNumber >= lineNumber)
                return cfln._locationCounter;
    return 0;
}

uint16_t Emulator::lastInstructionAddressAtSameFileLineNumber(uint16_t instructionAddress) const
{
    const QList<CodeFileLineNumber> &codeFileLineNumbers(_assembler->instructionsCodeFileLineNumbers());
    const CodeFileLineNumber cfln(instructionAddress, "", 0);
    auto it = std::lower_bound(codeFileLineNumbers.begin(), codeFileLineNumbers.end(), cfln,
                               [](const CodeFileLineNumber &lhs, const CodeFileLineNumber &rhs) -> bool { return lhs._locationCounter < rhs._locationCounter; });
    if (it == codeFileLineNumbers.end())
        return instructionAddress;
    QString filename = it->_codeFilename;
    int lineNumber = it->_currentCodeLineNumber;
    auto itNext = it + 1;
    while (itNext != codeFileLineNumbers.end() && itNext->_codeFilename == filename && itNext->_currentCodeLineNumber == lineNumber)
        it = itNext++;
    return it->_locationCounter;
}


int Emulator::findBreakpointInstructionAddressIndex(uint16_t instructionAddress) const
{
    int i = 0;
    while (i < _breakpoints.size() && _breakpoints.at(i).instructionAddress < instructionAddress)
        i++;
    return i;
}

void Emulator::insertIntoBreakpoints(const Breakpoint &breakpoint)
{
    int i = findBreakpointInstructionAddressIndex(breakpoint.instructionAddress);
    _breakpoints.insert(i, breakpoint);
}

const Emulator::Breakpoint *Emulator::findBreakpoint(const QString &filename, int lineNumber) const
{
    for (const Breakpoint &breakpoint : _breakpoints)
        if (breakpoint.filename == filename && breakpoint.lineNumber == lineNumber)
            return &breakpoint;
    return nullptr;
}

void Emulator::addBreakpoint(const QString &filename, int lineNumber)
{
    if (findBreakpoint(filename, lineNumber))
        return;
    insertIntoBreakpoints(Breakpoint(filename, lineNumber, mapFileLineNumberToInstructionAddress(filename, lineNumber)));
    emit breakpointChanged(filename, lineNumber);
}

void Emulator::toggleBreakpoint(const QString &filename, int lineNumber)
{
    bool found = false;
    for (int i = _breakpoints.length() - 1; i >= 0; i--)
        if (_breakpoints.at(i).filename == filename && _breakpoints.at(i).lineNumber == lineNumber)
        {
            _breakpoints.removeAt(i);
            found = true;
        }
    if (!found)
        insertIntoBreakpoints(Breakpoint(filename, lineNumber, mapFileLineNumberToInstructionAddress(filename, lineNumber)));
    emit breakpointChanged(filename, lineNumber);
}

void Emulator::clearBreakpoints()
{
    if (!_breakpoints.isEmpty())
    {
        _breakpoints.clear();
        emit breakpointChanged("", -1);
    }
}

bool Emulator::breakpointAtInstructionAddress(uint16_t instructionAddress) const
{
    int i = findBreakpointInstructionAddressIndex(instructionAddress);
    return i < _breakpoints.size() && _breakpoints.at(i).instructionAddress == instructionAddress;
}

QList<int> Emulator::breakpointLineNumbers(const QString &filename) const
{
    QList<int> lineNumbers;
    for (const Breakpoint &breakpoint : _breakpoints)
        if (breakpoint.filename == filename)
            lineNumbers.append(breakpoint.lineNumber);
    return lineNumbers;
}

void Emulator::setRuntimeBreakpoints(const QString &filename, const QList<int> &lineNumbers)
{
    if (lineNumbers.isEmpty() && _breakpoints.isEmpty())
        return;
    _breakpoints.clear();
    for (int i = 0; i < lineNumbers.size(); i++)
    {
        int lineNumber = lineNumbers.at(i);
        uint16_t instructionAddress = mapFileLineNumberToInstructionAddress(filename, lineNumber);
        if (instructionAddress == 0)
            continue;
        QString newFilename;
        int newLineNumber;
        mapInstructionAddressToFileLineNumber(instructionAddress, newFilename, newLineNumber);
        if (newLineNumber >= 0 && newFilename == filename)
            if (!findBreakpoint(filename, newLineNumber))
                insertIntoBreakpoints(Breakpoint(filename, newLineNumber, instructionAddress));
    }
    emit breakpointChanged("", -1);
}

void Emulator::clearBreakpointInstructionAddresses()
{
    for (Breakpoint &breakpoint : _breakpoints)
        breakpoint.instructionAddress = 0;
}


bool Emulator::profilingEnabled() const
{
    return settings().profilingEnabled();
}

void Emulator::startProfiling()
{
    if (!profilingEnabled())
        return;
    const Assembler::LocationCounterRange &locationCounterRange(assembler()->locationCounterRange());
    _processorModel->profiling().setGranularityShift(settings().profilingGranularityShift());
    _processorModel->setProfilingRange(locationCounterRange.lowest, locationCounterRange.highest);
    _processorModel->startProfiling();
}

void Emulator::getProfilingStatistics(QList<ProfilingLabelHitCount> &labelHitCounts)
{
    labelHitCounts.clear();
    const Profiling &profiling(_processorModel->profiling());
    if (!profilingEnabled() || !profiling.on)
        return;
    const CodeLabels &codeLabels(assembler()->codeLabels());
    QStringList allScopeLabels = assembler()->allScopeLabels();

    const QMap<QString, Assembler::ExpressionValue> &labelValues(codeLabels.values);
    for (const QString &label : labelValues.keys())
    {
        if (!label.contains('.') && !allScopeLabels.contains(label))
            continue;
        const Assembler::ExpressionValue &expressionValue(labelValues.value(label));
        if (!expressionValue.isValid())
            continue;
        if (expressionValue.intValue < profiling.programCounterLow || expressionValue.intValue >= profiling.programCounterHigh)
            continue;
        uint16_t address = expressionValue.intValue;
        labelHitCounts.append(ProfilingLabelHitCount(address, label));
    }
    if (labelHitCounts.isEmpty() && profiling.programCounterLow < profiling.programCounterHigh)
        labelHitCounts.append(ProfilingLabelHitCount(profiling.programCounterLow, "<TOP-LEVEL>"));
    std::sort(labelHitCounts.begin(), labelHitCounts.end(), [](const ProfilingLabelHitCount &a, const ProfilingLabelHitCount &b) { return a.address < b.address; });

    int labelHitCountIndex = 0;
    uint16_t programCounterLow, programCounterHigh;
    for (uint16_t pc = profiling.programCounterLow; pc < profiling.programCounterHigh; pc += profiling.granularitySize())
    {
        while (labelHitCountIndex + 1 < labelHitCounts.length() && labelHitCounts.at(labelHitCountIndex + 1).address <= pc)
            labelHitCountIndex++;
        int index = pc - profiling.programCounterLow;
        index >>= profiling.granularityShift;
        int hits = profiling.counts[index].hits;
        int cycles = profiling.counts[index].cycles;
        labelHitCounts[labelHitCountIndex].hitCount += hits;
        labelHitCounts[labelHitCountIndex].cycleCount += cycles;
    }
    labelHitCounts.removeIf([](const ProfilingLabelHitCount &labelHitCount) { return labelHitCount.hitCount == 0 && labelHitCount.cycleCount == 0; });
}


QList<int> Emulator::foldableBlocks(const QString &filename) const
{
    const CodeLabels &codeLabels(assembler()->codeLabels());

    QList<int> foldables;
    if (!codeLabels.scopes.contains(filename))
        return foldables;
    const QList<ScopeLabel> &scopeLabels(codeLabels.scopes.value(filename));
    for (int i = 0; i < scopeLabels.length(); i++)
    {
        int start;
        start = scopeLabels.at(i).lineNumber;
        if (i < scopeLabels.length() - 1)
        {
            int end = scopeLabels.at(i + 1).lineNumber - 1;
            if (start == end)
                continue;
        }
        foldables.append(start);
    }
    return foldables;
}


QString Emulator::scopeLabelAtLine(const QString &filename, int lineNumber) const
{
    const CodeLabels &codeLabels(assembler()->codeLabels());

    QString scopeLabel;
    if (codeLabels.scopes.contains(filename))
    {
        const QList<ScopeLabel> &scopeLabels(codeLabels.scopes.value(filename));
        for (int i = scopeLabels.length() - 1; i >= 0 && scopeLabel.isEmpty(); i--)
        {
            if (i > 0)
                Q_ASSERT(scopeLabels.at(i - 1).lineNumber <= scopeLabels.at(i).lineNumber);
            if (lineNumber >= scopeLabels.at(i).lineNumber)
                scopeLabel = scopeLabels.at(i).label;
        }
    }
    if (!scopeLabel.isEmpty())
        scopeLabel.append('.');
    return scopeLabel;
}

QStringListModel *Emulator::wordCompleterModel(const QString &filename, int lineNumber) const
{
    QStringList words;

    QString scopeLabel = scopeLabelAtLine(filename, lineNumber);
    const CodeLabels &codeLabels(assembler()->codeLabels());
    for (auto [label, value] : codeLabels.values.asKeyValueRange())
    {
        QString unscopedLabel(label);
        if (!scopeLabel.isEmpty() && unscopedLabel.startsWith(scopeLabel))
            unscopedLabel = unscopedLabel.mid(scopeLabel.length() - 1);
        if (!unscopedLabel.isEmpty() && unscopedLabel.at(0) != '@')
            words.append(unscopedLabel);
    }

    const MacroDefinitions &macroDefinitions(assembler()->macroDefinitions());
    words.append(macroDefinitions.keys());

    _wordCompleterModel->setStringList(words);
    return _wordCompleterModel;
}

QString Emulator::wordCompletion(const QString &word) const
{
    int wordLen = word.length();
    QString completion;
    for (const QString &label : _wordCompleterModel->stringList())
    {
        if (label.left(wordLen) != word)
            continue;
        QString newCompletion = label.mid(wordLen);
        if (newCompletion.isEmpty())
            continue;
        else if (completion.isEmpty())
            completion = newCompletion;
        else
        {
            int maxLen = std::min(newCompletion.length(), completion.length());
            int commonLen = 0;
            while (commonLen < maxLen && newCompletion.at(commonLen) == completion.at(commonLen))
                commonLen++;
            completion = newCompletion.left(commonLen);
            if (completion.isEmpty())
                break;
        }
    }
    return completion;
}


//
// AssemblerBreakpointProvider Class
//
void AssemblerBreakpointProvider::clearBreakpoints() /*override*/
{
    _emulator->clearBreakpoints();
}

void AssemblerBreakpointProvider::addBreakpoint(const QString &filename, int lineNumber) /*override*/
{
    _emulator->addBreakpoint(filename, lineNumber);
}


//
// ProcessorBreakpointProvider Class
//
bool ProcessorBreakpointProvider::breakpointAt(uint16_t instructionAddress) const /*override*/
{
    return _emulator->breakpointAtInstructionAddress(instructionAddress) != 0;
}

uint16_t ProcessorBreakpointProvider::lastInstructionAddressAtSameFileLineNumber(uint16_t instructionAddress) const
{
    return _emulator->lastInstructionAddressAtSameFileLineNumber(instructionAddress);
}


//
// WatchModel Class
//
WatchModel::WatchModel(MemoryModel *memoryModel, Emulator *emulator)
    : QAbstractTableModel(memoryModel),
    memoryModel(memoryModel),
    emulator(emulator)
{
    Q_ASSERT(memoryModel);
    Q_ASSERT(emulator);
    watchInfos.resize(_rowCount);
    connect(memoryModel, &MemoryModel::dataChanged, this, &WatchModel::memoryModelDataChanged);
}

int WatchModel::rowCount(const QModelIndex &parent) const /*override*/
{
    return _rowCount;
}

int WatchModel::columnCount(const QModelIndex &parent) const /*override*/
{
    return 6;
}

QVariant WatchModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const /*override*/
{
    if (!index.isValid())
        return QVariant();
    uint16_t memoryAddress = watchInfos.at(index.row()).memoryAddress;
    if (index.column() >= 2)
    {
        QModelIndex memoryIndex = memoryModel->addressToIndex(memoryAddress + index.column() - 2);
        return memoryModel->data(memoryIndex, role);
    }
    switch (role)
    {
    case Qt::TextAlignmentRole:
        if (index.column() == 0)
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        else if (index.column() == 1)
            return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
        break;
    case Qt::DisplayRole:
    case Qt::EditRole: {
        uint16_t address = watchInfos.at(index.row()).memoryAddress;
        if (index.column() == 0)
            return watchInfos.at(index.row()).symbol;
        else if (index.column() == 1)
            return address;
        break;
    }
    default:
        break;
    }
    return QVariant();
}

QVariant WatchModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const /*override*/
{
    const QStringList columnNames{ "symbol", "address", "(addr)", "(ad+1)", "(ad+2)", "(ad+3)" };

    if (orientation == Qt::Orientation::Horizontal)
    {
        if (section < 0 || section >= columnNames.size())
            return QVariant();
        if (role == Qt::DisplayRole)
            return columnNames.at(section);
    }
    return QVariant();
}

bool WatchModel::setData(const QModelIndex &index, const QVariant &value, int role /*= Qt::EditRole*/) /*override*/
{
    if (!index.isValid())
        return false;
    uint16_t memoryAddress = watchInfos.at(index.row()).memoryAddress;
    if (index.column() >= 2)
    {
        QModelIndex memoryIndex = memoryModel->addressToIndex(memoryAddress + index.column() - 2);
        return memoryModel->setData(memoryIndex, value, role);
    }
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column() == 0)
        {
            QString label(value.toString());
            if (watchInfos[index.row()].symbol != label)
            {
                watchInfos[index.row()].symbol = label;
                emit dataChanged(index, index, { role });
            }
            if (!label.isEmpty())
            {
                QModelIndex addressIndex(index.siblingAtColumn(1));
                Assembler::ExpressionValue value(emulator->assembler()->codeLabelValue(label));
                if (value.isValid())
                    setData(addressIndex, value.intValue);
            }
            return true;
        }
        else if (index.column() == 1)
        {
            int address = value.toInt();
            if (watchInfos[index.row()].memoryAddress != address)
            {
                watchInfos[index.row()].memoryAddress = address;
                emit dataChanged(index, index.siblingAtColumn(columnCount() - 1), { role });
            }
            QModelIndex labelIndex(index.siblingAtColumn(0));
            QString label(labelIndex.data().toString());
            if (!label.isEmpty())
            {
                Assembler::ExpressionValue value(emulator->assembler()->codeLabelValue(label));
                if (!value.isValid() || value.intValue != address)
                    setData(labelIndex, QString());
            }
            return true;
        }
    }
    return false;
}

Qt::ItemFlags WatchModel::flags(const QModelIndex &index) const
{
    if (index.column() >= 2)
    {
        uint16_t memoryAddress = watchInfos.at(index.row()).memoryAddress;
        QModelIndex memoryIndex = memoryModel->addressToIndex(memoryAddress + index.column() - 2);
        return memoryModel->flags(memoryIndex);
    }
    Qt::ItemFlags f(QAbstractTableModel::flags(index));
    f |= Qt::ItemFlag::ItemIsEditable;
    if (index.column() == 0)
        f |= Qt::ItemIsDropEnabled;
    return f;
}

Qt::DropActions WatchModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

bool WatchModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return false;
    Q_UNUSED(row)
    Q_UNUSED(column)
    int dropRow = parent.row(), dropColumn = parent.column();
    Q_UNUSED(dropRow)
    return dropColumn == 0 && data->hasText();
}

bool WatchModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!parent.isValid())
        return false;
    if (!canDropMimeData(data, action, row, column, parent))
        return false;
    if (action == Qt::IgnoreAction)
        return true;
    int dropRow = parent.row(), dropColumn = parent.column();
    QString text = data->text();
    bool success = setData(index(dropRow, dropColumn), text);
    return success;
}

void WatchModel::recalculateAllSymbols()
{
    for (int row = 0; row < rowCount(); row++)
        setData(index(row, 0), index(row, 0).data(Qt::EditRole));
}

/*slot*/ void WatchModel::memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles /*= QList<int>()*/)
{
    if (!(roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole) || roles.contains(Qt::ForegroundRole)))
        return;
    uint16_t lowAddress(memoryModel->indexToAddress(topLeft)), highAddress(memoryModel->indexToAddress(bottomRight));
    if (lowAddress == 0 && highAddress == 0xffff)
    {
        emit dataChanged(index(0, 2), index(rowCount() - 1, columnCount() - 1), roles);
        return;
    }
    for (int i = 0; i < rowCount(); i++)
    {
        const WatchInfo &watchInfo(watchInfos.at(i));
        if (!watchInfo.inUse())
            continue;
        QModelIndex thisTopLeft, thisBottomRight;
        for (int col = 2; col < columnCount(); col++)
            if (watchInfo.memoryAddress + col - 2 >= lowAddress && watchInfo.memoryAddress + col - 2 <= highAddress)
            {
                if (!thisTopLeft.isValid())
                    thisTopLeft = index(i, col);
                thisBottomRight = index(i, col);
            }
        if (thisTopLeft.isValid() && thisBottomRight.isValid())
            emit dataChanged(thisTopLeft, thisBottomRight, roles);
    }
}
