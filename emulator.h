#ifndef EMULATOR_H
#define EMULATOR_H

#include <QObject>
#include <QPair>
#include <QStringListModel>

#include "processormodel.h"
#include "assembler.h"

class Emulator;
extern Emulator *g_emulator;

class AssemblerBreakpointProvider;

//
// Emulator Class
//
class Emulator : public QObject
{
    Q_OBJECT
public:
    explicit Emulator(QObject *parent = nullptr);
    ~Emulator();

    struct Breakpoint
    {
        QString filename;
        int lineNumber = -1;
        uint16_t instructionAddress = 0;

        Breakpoint() {}
        Breakpoint(const QString &_filename, int _lineNumber, uint16_t _instructionAddress)
        {
            filename = _filename;
            lineNumber = _lineNumber;
            instructionAddress = _instructionAddress;
        }
    };

    ProcessorModel *processorModel() const { return _processorModel; };

    Assembler *assembler() const { return _assembler; };

    const uint16_t runStartAddress() const;
    void mapInstructionAddressToFileLineNumber(uint16_t instructionAddress, QString &filename, int &lineNumber) const;
    uint16_t mapFileLineNumberToInstructionAddress(const QString &filename, int lineNumber, bool exact = false) const;
    uint16_t lastInstructionAddressAtSameFileLineNumber(uint16_t instructionAddress) const;

    const Breakpoint *findBreakpoint(const QString &filename, int lineNumber) const;
    void addBreakpoint(const QString &filename, int lineNumber);
    void toggleBreakpoint(const QString &filename, int lineNumber);
    void clearBreakpoints();
    bool breakpointAtInstructionAddress(uint16_t instructionAddress) const;
    QList<int> breakpointLineNumbers(const QString &filename) const;
    void setRuntimeBreakpoints(const QString &filename, const QList<int> &lineNumbers);
    void clearBreakpointInstructionAddresses();

    struct ProfilingLabelHitCount
    {
        uint16_t address;
        QString label;
        int hitCount = 0, cycleCount = 0;
        ProfilingLabelHitCount(uint16_t _address, const QString &_label) { address = _address; label = _label; }
    };
    bool profilingEnabled() const;

    void startProfiling();
    void getProfilingStatistics(QList<ProfilingLabelHitCount> &labelHitCounts);

    QList<int> foldableBlocks(const QString &filename) const;

signals:
    void breakpointChanged(const QString &filename, int lineNumber);

private:
    int findBreakpointInstructionAddressIndex(uint16_t instructionAddress) const;
    void insertIntoBreakpoints(const Breakpoint &breakpoint);

public:
    QString wordCompletion(const QString &word) const;
    QStringListModel *wordCompleterModel(const QString &filename, int lineNumber) const;

private:
    ProcessorModel *_processorModel;
    uint8_t *_memory;
    Instruction *_instructions;
    QList<Breakpoint> _breakpoints;
    IProcessorBreakpointProvider *processorBreakpointProvider;

    Assembler *_assembler;
    AssemblerBreakpointProvider *assemblerBreakpointProvider;

    bool _profilingEnabled;

    QStringListModel *_wordCompleterModel;

    QString scopeLabelAtLine(const QString &filename, int lineNumber) const;
};


//
// AssemblerBreakpointProvider Class
//
class AssemblerBreakpointProvider : public IAssemblerBreakpointProvider
{
public:
    AssemblerBreakpointProvider(Emulator *emulator) : _emulator(emulator) {}

private:
    Emulator *_emulator;
    Emulator *emulator() const { return _emulator; }
    void clearBreakpoints() override;
    void addBreakpoint(const QString &filename, int lineNumber) override;
};


//
// ProcessorBreakpointProvider Class
//
class ProcessorBreakpointProvider : public IProcessorBreakpointProvider
{
public:
    ProcessorBreakpointProvider(Emulator *emulator) : _emulator(emulator) {}

private:
    Emulator *_emulator;
    Emulator *emulator() const { return _emulator; }
    bool breakpointAt(uint16_t instructionAddress) const override;
    uint16_t lastInstructionAddressAtSameFileLineNumber(uint16_t instructionAddress) const override;
};


//
// WatchModel Class
//
class WatchModel : public QAbstractTableModel
{
    Q_OBJECT

    // QAbstractItemModel interface
public:
    WatchModel(MemoryModel *memoryModel, Emulator *emulator);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    void recalculateAllSymbols();

private:
    struct WatchInfo
    {
        QString symbol;
        uint16_t memoryAddress;
        uint8_t value0, value1;

        WatchInfo() { symbol = QString(); memoryAddress = 0; value0 = value1 = 0; }
        bool inUse() const { return memoryAddress != 0; }
    };

    static constexpr int _rowCount = 6;
    MemoryModel *memoryModel;
    Emulator *emulator;
    QList<WatchInfo> watchInfos;

public slots:
    void memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
};

#endif // EMULATOR_H
