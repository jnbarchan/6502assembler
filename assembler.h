#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <QMetaEnum>
#include <QObject>
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

    const QList<Instruction> &instructions() const;
    const QList<int> &instructionsCodeLineNumbers() const;

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

signals:
    void sendMessageToConsole(const QString &message) const;
    void currentCodeLineNumberChanged(int lineNumber);

private:
    enum AssembleState { NotStarted, Pass1, Pass2, Assembled };
    AssembleState _assembleState;

    const QStringList _directives{ ".byte", };

    QTextStream *_codeStream;
    QStringList _codeLines;

    QString _currentLine, _currentToken;
    QTextStream _currentLineStream;
    int _currentCodeLineNumber;

    QList<Instruction> _instructions;
    int _currentCodeInstructionNumber;
    QList<int> _instructionsCodeLineNumbers;

    QMap<QString, int> _codeLabels;
    QString _currentCodeLabelScope;

    bool _codeLabelRequiresColon = true;

    AssembleState assembleState() const;
    void setAssembleState(AssembleState newAssembleState);
    void debugMessage(const QString &message) const;
    QString scopedLabelName(const QString &label) const;
    void assignLabelValue(const QString &scopedLabel, int value);
    bool assemblePass();
    bool assembleNextStatement(Opcodes &opcode, OpcodeOperand &operand, bool &hasOpcode, bool &eof);
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
