#include "emulator.h"

using CodeFileLineNumber = Assembler::CodeFileLineNumber;
using CodeLabels = Assembler::CodeLabels;
using ScopeLabel = Assembler::ScopeLabel;

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

int Emulator::mapFileLineNumberToInstructionAddress(const QString &filename, int lineNumber, bool exact /*= false*/) const
{
    const QList<CodeFileLineNumber> &codeFileLineNumbers(_assembler->instructionsCodeFileLineNumbers());
    for (const CodeFileLineNumber &cfln : codeFileLineNumbers)
        if (cfln._codeFilename == filename)
            if (exact ? cfln._currentCodeLineNumber == lineNumber : cfln._currentCodeLineNumber >= lineNumber)
                return cfln._locationCounter;
    return -1;
}


int Emulator::findBreakpointIndex(uint16_t instructionAddress) const
{
    int i = 0;
    while (i < _breakpoints.size() && _breakpoints.at(i) < instructionAddress)
        i++;
    return i;
}

int Emulator::findBreakpoint(const QString &filename, int lineNumber) const
{
    // int instructionAddress = mapFileLineNumberToInstructionAddress(filename, lineNumber, true);
    // if (instructionAddress < 0)
    //     return -1;
    // int i = findBreakpointIndex(instructionAddress);
    // return i < _breakpoints.size() && _breakpoints.at(i) == instructionAddress ? instructionAddress : -1;

    for (int i = 0; i < _breakpoints.size(); i++)
    {
        QString newFilename;
        int newLineNumber;
        mapInstructionAddressToFileLineNumber(_breakpoints.at(i), newFilename, newLineNumber);
        if (newFilename == filename && newLineNumber == lineNumber)
            return _breakpoints.at(i);
    }
    return -1;
}

void Emulator::toggleBreakpoint(const QString &filename, int lineNumber)
{
    int instructionAddress = mapFileLineNumberToInstructionAddress(filename, lineNumber);
    if (instructionAddress < 0)
        return;
    int i = findBreakpointIndex(instructionAddress);
    if (i < _breakpoints.size() && _breakpoints.at(i) == instructionAddress)
        _breakpoints.removeAt(i);
    else
        _breakpoints.insert(i, instructionAddress);
    emit breakpointChanged(instructionAddress);
}

void Emulator::clearBreakpoints()
{
    if (!_breakpoints.isEmpty())
    {
        _breakpoints.clear();
        emit breakpointChanged(-1);
    }
}

void Emulator::addAssemblerBreakpoint(uint16_t instructionAddress)
{
    int i = findBreakpointIndex(instructionAddress);
    if (i < _breakpoints.size() && _breakpoints.at(i) == instructionAddress)
        return;
    _breakpoints.insert(i, instructionAddress);
    emit breakpointChanged(instructionAddress);
}

void Emulator::clearAssemblerBreakpoints()
{
    clearBreakpoints();
}

bool Emulator::breakpointAt(uint16_t instructionAddress)
{
    return _breakpoints.contains(instructionAddress);
}


QString Emulator::wordCompletion(const QString &word, const QString &filename, int lineNumber) const
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

    int wordLen = word.length();
    QString completion;
    for (auto [label, value] : codeLabels.values.asKeyValueRange())
    {
        int startAt = 0;
        if (!scopeLabel.isEmpty() && label.startsWith(scopeLabel))
            startAt = scopeLabel.length() - 1;
        if (label.mid(startAt, wordLen) != word)
            continue;
        QString newCompletion = label.mid(startAt + wordLen);
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
                return QString();
        }
    }
    return completion;
}


//
// AssemblerBreakpointProvider Class
//
void AssemblerBreakpointProvider::clearBreakpoints() /*override*/
{
    _emulator->clearAssemblerBreakpoints();
}

void AssemblerBreakpointProvider::addBreakpoint(uint16_t instructionAddress) /*override*/
{
    _emulator->addAssemblerBreakpoint(instructionAddress);
}


//
// ProcessorBreakpointProvider Class
//
bool ProcessorBreakpointProvider::breakpointAt(uint16_t instructionAddress) const /*override*/
{
    return _emulator->breakpointAt(instructionAddress);
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
    QModelIndex memoryIndex;
    if (index.column() >= 2)
        memoryIndex = memoryModel->addressToIndex(memoryAddress + index.column() - 2);
    switch (role)
    {
    case Qt::TextAlignmentRole:
        return QVariant((index.column() == 0 ? Qt::AlignLeft : Qt::AlignCenter) | Qt::AlignVCenter);
    case Qt::DisplayRole:
    case Qt::EditRole: {
        uint16_t address = watchInfos.at(index.row()).memoryAddress;
        switch (index.column())
        {
        case 0: return watchInfos.at(index.row()).symbol;
        case 1: return address;
        default: return memoryModel->data(memoryIndex, role);
        }
        break;
    }
    case Qt::ForegroundRole:
        if (index.column() >= 2)
            return memoryModel->data(memoryIndex, role);
        break;
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
    QModelIndex memoryIndex;
    if (index.column() >= 2)
        memoryIndex = memoryModel->addressToIndex(memoryAddress + index.column() - 2);
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
        else
        {
            bool ok;
            int val = value.toUInt(&ok);
            if (!ok)
                return false;
            return memoryModel->setData(memoryIndex, static_cast<uint8_t>(val), role);
        }
    }
    return false;
}

Qt::ItemFlags WatchModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f(QAbstractTableModel::flags(index));
    f |= Qt::ItemFlag::ItemIsEditable;
    return f;
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
