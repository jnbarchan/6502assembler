#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <QFile>
#include <QMetaEnum>
#include <QObject>
#include <QStack>
#include <QTextStream>

#include "assembly.h"

using Operation = Assembly::Operation;
using AddressingMode = Assembly::AddressingMode;
using AddressingModeFlag = Assembly::AddressingModeFlag;
using Instruction = Assembly::Instruction;
using InstructionInfo = Assembly::InstructionInfo;
using InternalJSRs = Assembly::InternalJSRs;
using InternalVECs = Assembly::InternalVECs;


class IAssemblerBreakpointProvider;

//
// Assembler Class
//
class Assembler : public QObject
{
    Q_OBJECT
public:
    explicit Assembler(QObject *parent = nullptr);

    void setAssemblerBreakpointProvider(IAssemblerBreakpointProvider *provider);

    struct CodeFileLineNumber
    {
        uint16_t _locationCounter;
        QString _codeFilename;
        int _currentCodeLineNumber;

        CodeFileLineNumber(uint16_t locationCounter, QString filename, int lineNumber)
        {
            _locationCounter = locationCounter;
            _codeFilename = filename;
            _currentCodeLineNumber = lineNumber;
        }
    };

    struct ScopeLabel
    {
        QString label;
        int lineNumber;
        ScopeLabel(const QString &_label, int _lineNumber) { label = _label; lineNumber = _lineNumber; }
    };
    struct CodeLabels
    {
        QMap<QString, int> values;
        QMap<QString, QList<ScopeLabel> > scopes;
    };

    const uint16_t defaultLocationCounter() const;

    const Instruction *instructions() const;
    void setInstructions(Instruction *newInstructions);
    const QList<CodeFileLineNumber> &instructionsCodeFileLineNumbers() const;

    bool needsAssembling() const;
    void setNeedsAssembling();

    int currentCodeLineNumber() const;
    void setCurrentCodeLineNumber(int newCurrentCodeLineNumber);
    uint16_t locationCounter() const;
    void setLocationCounter(uint16_t newLocationCounter);

    const CodeLabels &codeLabels() const;
    int codeLabelValue(const QString &key) const;
    void setCodeLabelValue(const QString &key, int value);

    void setCode(QTextStream *codeStream);

    void restart(bool assemblePass2 = false);
    void assemble();

    const QStringList &codeIncludeDirectories() const;
    void setCodeIncludeDirectories(const QStringList &newCodeIncludeDirectories);

    uint8_t *memory() const;
    void setMemory(uint8_t *newMemory);

signals:
    void sendMessageToConsole(const QString &message, Qt::GlobalColor colour = Qt::transparent) const;
    void currentCodeLineNumberChanged(const QString &filename, int lineNumber);

private:
    enum AssembleState { NotStarted, Pass1, Pass2, Assembled };
    AssembleState _assembleState;

    struct CodeFileState
    {
        QString filename;
        int lineNumber;
        QFile *file = nullptr;
        QTextStream *stream = nullptr;
        QStringList lines;
    };
    CodeFileState currentFile;
    QStack<CodeFileState> codeFileStateStack;
    QStringList _codeIncludeDirectories;
    QStringList _includedFilePaths;

    struct CodeLineState
    {
        QString currentToken;
        QString line;
        QTextStream lineStream;
    };
    CodeLineState currentLine;
    QString &currentToken /* = currentLine.currentToken */;

    uint8_t *_memory;
    Instruction *_instructions;
    uint16_t _locationCounter;
    const uint16_t _defaultLocationCounter = 0xc000;
    QList<CodeFileLineNumber> _instructionsCodeFileLineNumbers;
    IAssemblerBreakpointProvider *assemblerBreakpointProvider;

    CodeLabels _codeLabels;
    QString _currentCodeLabelScope;

    bool _codeLabelRequiresColon = true;

    AssembleState assembleState() const;
    void setAssembleState(AssembleState newAssembleState);
    void debugMessage(const QString &message) const;
    void assemblerWarningMessage(const QString &message) const;
    void assemblerErrorMessage(const QString &message) const;
    QString scopedLabelName(const QString &label) const;
    void assignLabelValue(const QString &scopedLabel, bool isLabel, int value);
    void cleanup(bool assemblePass2 = false);
    void addInstructionsCodeFileLineNumber(const CodeFileLineNumber &cfln);
    void assemblePass();
    void assembleNextStatement(Operation &operation, AddressingMode &mode, uint16_t &arg, bool &hasOperation, bool &eof);
    void assembleNextStatement(Operation &operation, AddressingMode &mode, uint16_t &arg, bool &hasOperation);
    void assembleDirective();
    QString findIncludeFilePath(const QString &includeFilename);
    void startIncludeFile(const QString &includeFilename);
    void endIncludeFile();
    void closeIncludeFile();
    bool getNextLine();
    QString peekNextToken(bool wantOperator = false);
    bool getNextToken(bool wantOperator = false);
    int getTokensExpressionValueAsInt(bool &ok, bool allowForIndirectAddressing = false, bool *indirectAddressingMetCloseParen = nullptr);
    bool tokenIsDirective() const;
    bool tokenIsLabel(bool isDefinition = false) const;
    bool tokenIsInt() const;
    int tokenToInt(bool *ok) const;
    int tokenValueAsInt(bool *ok) const;
    bool tokenIsString() const;
    QString tokenToString(bool *ok) const;
};


//
// AssemblerError Class
//
class AssemblerError : public std::runtime_error
{
public:
    AssemblerError(const QString &msg);
};


//
// IAssemblerBreakpointProvider Class
//
class IAssemblerBreakpointProvider
{
public:
    virtual ~IAssemblerBreakpointProvider() = default;
    virtual void clearBreakpoints() = 0;
    virtual void addBreakpoint(uint16_t instructionAddress) = 0;
};

#endif // ASSEMBLER_H
