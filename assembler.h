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

    void setCodeLabel(const QString &key, int value);

    void setCode(QTextStream *codeStream);

    void restart(bool assemblePass2 = false);
    void assemble();

    const QStringList &codeIncludeDirectories() const;
    void setCodeIncludeDirectories(const QStringList &newCodeIncludeDirectories);

signals:
    void sendMessageToConsole(const QString &message) const;
    void currentCodeLineNumberChanged(const QString &filename, int lineNumber);

private:
    enum AssembleState { NotStarted, Pass1, Pass2, Assembled };
    AssembleState _assembleState;

    QString _codeFilename;
    int _currentCodeLineNumber;
    QFile *_codeFile;
    QTextStream *_codeStream;
    QStringList _codeLines;
    struct CodeInputState
    {
        QString _codeFilename;
        int _currentCodeLineNumber;
        QFile *_codeFile = nullptr;
        QTextStream *_codeStream = nullptr;
        QStringList _codeLines;
    };
    QStack<CodeInputState> codeInputStateStack;
    QStringList _codeIncludeDirectories;

    QString _currentLine, _currentToken;
    QTextStream _currentLineStream;

    QList<Instruction> *_instructions;
    int _currentCodeInstructionNumber;
    QList<CodeFileLineNumber> _instructionsCodeFileLineNumbers;

    QMap<QString, int> _codeLabels;
    QString _currentCodeLabelScope;

    bool _codeLabelRequiresColon = true;

    AssembleState assembleState() const;
    void setAssembleState(AssembleState newAssembleState);
    void debugMessage(const QString &message) const;
    void assemblerWarning(const QString &message) const;
    void assemblerError(const QString &message) const;
    QString scopedLabelName(const QString &label) const;
    void assignLabelValue(const QString &scopedLabel, int value);
    bool assemblePass();
    bool assembleNextStatement(Opcodes &opcode, OpcodeOperand &operand, bool &hasOpcode, bool &eof);
    QString findIncludeFilePath(const QString &includeFilename);
    bool startIncludeFile(const QString &includeFilename);
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

#endif // ASSEMBLER_H
