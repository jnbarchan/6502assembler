#include "emulator.h"

using CodeFileLineNumber = Assembler::CodeFileLineNumber;

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

    _queueChangedSignals = false;
    connect(&pendingSignalsTimer, &QTimer::timeout, this, &Emulator::processQueuedChangedSignals);
}

Emulator::~Emulator()
{
    delete processorBreakpointProvider;
    delete assemblerBreakpointProvider;
}

const uint16_t Emulator::runStartAddress() const
{
    int value;
    for (const QString &label : { "reset", "start", "START", "main", "MAIN", })
        if ((value = assembler()->codeLabelValue(label)) >= 0)
            return value;
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
    //TEMPORARY?
    int i = findBreakpointIndex(instructionAddress);
    if (i < _breakpoints.size() && _breakpoints.at(i) == instructionAddress)
        return;
    _breakpoints.insert(i, instructionAddress);
    emit breakpointChanged(instructionAddress);
}

void Emulator::clearAssemblerBreakpoints()
{
    //TEMPORARY?
    clearBreakpoints();
}

bool Emulator::breakpointAt(uint16_t instructionAddress)
{
    return _breakpoints.contains(instructionAddress);
}


bool Emulator::queueChangedSignals() const
{
    return _queueChangedSignals;
}

void Emulator::setQueueChangedSignals(bool newQueueChangedSignals)
{
    _queueChangedSignals = newQueueChangedSignals;
}

void Emulator::startQueuingChangedSignals()
{
    setQueueChangedSignals(true);
    pendingSignalsTimer.start(100);
}

void Emulator::endQueuingChangedSignals()
{
    pendingSignalsTimer.stop();
    setQueueChangedSignals(false);
    processQueuedChangedSignals();
}

void Emulator::enqueueQueuedChangedSignal(const QueuedChangeSignal &sig)
{
    for (int i = _changedSignalsQueue.size() - 1; i >= 0; i--)
    {
        const QueuedChangeSignal &other(_changedSignalsQueue.at(i));
        if (other.tag != sig.tag)
            continue;
        bool remove = false;
        switch (sig.tag)
        {
        case QueuedChangeSignal::CurrentCodeLineNumberChanged:
            if (other.codeLine.filename == sig.codeLine.filename)
                remove = true;
            break;
        case QueuedChangeSignal::CurrentInstructionAddressChanged:
            remove = true;
            break;
        case QueuedChangeSignal::MemoryModelDataChanged:
            if (other.memory.topLeft == sig.memory.topLeft && other.memory.bottomRight == sig.memory.bottomRight)
            {
                const QList<int> &otherRoles(other.memory.roles), &sigRoles(sig.memory.roles);
                if (otherRoles == sigRoles || sigRoles.isEmpty() || (otherRoles.size() == 1 && sigRoles.contains(otherRoles.at(0))))
                    remove = true;
                else if (otherRoles.isEmpty() || (sigRoles.size() == 1 && otherRoles.contains(sigRoles.at(0))))
                    continue;
            }
            break;
        case QueuedChangeSignal::RegisterChanged:
            if (other._register.spn == sig._register.spn)
                remove = true;
            break;
        }
        if (remove)
        {
            _changedSignalsQueue.removeAt(i);
            if (sig.tag != QueuedChangeSignal::MemoryModelDataChanged)
                break;
        }
    }
    _changedSignalsQueue.enqueue(sig);
}

/*slot*/ void Emulator::processQueuedChangedSignals()
{
    bool currentQueueChangedSignals = queueChangedSignals();
    setQueueChangedSignals(false);
    while (!_changedSignalsQueue.isEmpty())
    {
        QueuedChangeSignal sig = _changedSignalsQueue.dequeue();
        emit processQueuedChangedSignal(sig);
    }
    setQueueChangedSignals(currentQueueChangedSignals);
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
    return 4;
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
    const QStringList columnNames{ "symbol", "address", "(addr)", "(addr+1)" };

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
    if (index.column() >= 2)
        return false;
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
            QModelIndex addressIndex(index.siblingAtColumn(1));
            if (!label.isEmpty())
                setData(addressIndex, emulator->assembler()->codeLabelValue(label));
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
            if (!label.isEmpty() && emulator->assembler()->codeLabelValue(label) != address)
                setData(labelIndex, QString());
            return true;
        }
    }
    return false;
}

Qt::ItemFlags WatchModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f(QAbstractTableModel::flags(index));
    if (index.column() < 2)
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
    if (emulator->queueChangedSignals())
        return;
    if (!(roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::EditRole) || roles.contains(Qt::ForegroundRole)))
        return;
    uint16_t lowAddress(memoryModel->indexToAddress(topLeft)), highAddress(memoryModel->indexToAddress(bottomRight));
    if (lowAddress == 0 && highAddress == 0xffff)
    {
        emit dataChanged(index(0, 2), index(rowCount() - 1, 3), roles);
        return;
    }
    for (int i = 0; i < rowCount(); i++)
    {
        const WatchInfo &watchInfo(watchInfos.at(i));
        if (!watchInfo.inUse())
            continue;
        QModelIndex thisTopLeft, thisBottomRight;
        if (watchInfo.memoryAddress >= lowAddress && watchInfo.memoryAddress <= highAddress)
            thisTopLeft = thisBottomRight = index(i, 2);
        if (watchInfo.memoryAddress + 1 >= lowAddress && watchInfo.memoryAddress + 1 <= highAddress)
        {
            thisBottomRight = index(i, 3);
            if (!thisTopLeft.isValid())
                thisTopLeft = thisBottomRight;
        }
        if (thisTopLeft.isValid() && thisBottomRight.isValid())
            emit dataChanged(thisTopLeft, thisBottomRight, roles);
    }
}
