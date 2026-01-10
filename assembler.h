#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <QFile>
#include <QMetaEnum>
#include <QObject>
#include <QStack>
#include <QTextStream>

#include "assembly.h"

using Opcodes = Assembly::Opcodes;
using AddressingMode = Assembly::AddressingMode;
using OpcodeOperand = Assembly::OpcodeOperand;
using Instruction = Assembly::Instruction;
using InternalJSRs = Assembly::InternalJSRs;


class Assembler : public QObject
{
    Q_OBJECT
public:
    explicit Assembler(QObject *parent = nullptr);

    struct CodeFileLineNumber
    {
        QString _codeFilename;
        int _currentCodeLineNumber;

        CodeFileLineNumber(QString filename, int lineNumber)
        {
            _codeFilename = filename;
            _currentCodeLineNumber = lineNumber;
        }
    };

    const QList<Instruction> *instructions() const;
    void setInstructions(QList<Instruction> *newInstructions);
    const QList<CodeFileLineNumber> &instructionsCodeFileLineNumbers() const;

    bool needsAssembling() const;
    void setNeedsAssembling();

    int currentCodeLineNumber() const;
    void setCurrentCodeLineNumber(int newCurrentCodeLineNumber);
    int currentCodeInstructionNumber() const;
    void setCurrentCodeInstructionNumber(int newCurrentCodeInstructionNumber);

    QList<uint16_t> *breakpoints() const;
    void setBreakpoints(QList<uint16_t> *newBreakpoints);

    void setCodeLabel(const QString &key, int value);

    void setCode(QTextStream *codeStream);

    void restart(bool assemblePass2 = false);
    void assemble();

    const QStringList &codeIncludeDirectories() const;
    void setCodeIncludeDirectories(const QStringList &newCodeIncludeDirectories);

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

    QList<Instruction> *_instructions;
    int _currentCodeInstructionNumber;
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
    void assemblePass();
    void assembleNextStatement(Opcodes &opcode, OpcodeOperand &operand, bool &hasOpcode, bool &eof);
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
};


class AssemblerError : public std::runtime_error
{
public:
    AssemblerError(const QString &msg);
};

#endif // ASSEMBLER_H
