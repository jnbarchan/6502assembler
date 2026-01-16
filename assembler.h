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


class Assembler : public QObject
{
    Q_OBJECT
public:
    explicit Assembler(QObject *parent = nullptr);

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

    QList<uint16_t> *breakpoints() const;
    void setBreakpoints(QList<uint16_t> *newBreakpoints);

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
    const uint16_t _defaultLocationCounter = 0xC000;
    QList<CodeFileLineNumber> _instructionsCodeFileLineNumbers;
    QList<uint16_t> *_breakpoints;

    QMap<QString, int> _codeLabels;
    QString _currentCodeLabelScope;

    bool _codeLabelRequiresColon = true;

    AssembleState assembleState() const;
    void setAssembleState(AssembleState newAssembleState);
    void debugMessage(const QString &message) const;
    void assemblerWarningMessage(const QString &message) const;
    void assemblerErrorMessage(const QString &message) const;
    QString scopedLabelName(const QString &label) const;
    void assignLabelValue(const QString &scopedLabel, int value);
    void cleanup();
    void assemblePass();
    void assembleNextStatement(Operation &operation, AddressingMode &mode, uint16_t &arg, bool &hasOperation, bool &eof);
    QString findIncludeFilePath(const QString &includeFilename);
    void startIncludeFile(const QString &includeFilename);
    void endIncludeFile();
    void closeIncludeFile();
    bool getNextLine();
    bool getNextToken(bool wantOperator = false);
    int getTokensExpressionValueAsInt(bool &ok, bool allowForIndirectAddressing = false, bool *indirectAddressingMetCloseParen = nullptr);
    bool tokenIsDirective() const;
    bool tokenIsLabel() const;
    bool tokenIsInt() const;
    int tokenToInt(bool *ok) const;
    int tokenValueAsInt(bool *ok) const;
    bool tokenIsString() const;
    QString tokenToString(bool *ok) const;
};


class AssemblerError : public std::runtime_error
{
public:
    AssemblerError(const QString &msg);
};

#endif // ASSEMBLER_H
