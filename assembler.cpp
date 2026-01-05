#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStack>

#include "assembler.h"


//
// Assembler Class
//

Assembler::Assembler(QObject *parent)
    : QObject{parent},
    currentToken(currentLine.currentToken)
{
    currentFile.filename.clear();
    currentFile.lineNumber = -1;
    currentFile.file = nullptr;
    currentFile.stream = nullptr;
    currentFile.lines.clear();
    codeFileStateStack.clear();
    _codeIncludeDirectories.clear();
    _assembleState = AssembleState::NotStarted;
}

Assembler::AssembleState Assembler::assembleState() const
{
    return _assembleState;
}

void Assembler::setAssembleState(AssembleState newAssembleState)
{
    _assembleState = newAssembleState;
}

bool Assembler::needsAssembling() const
{
    return _assembleState != Assembled;
}

void Assembler::setNeedsAssembling()
{
    setAssembleState(AssembleState::NotStarted);
}

int Assembler::currentCodeLineNumber() const
{
    return currentFile.lineNumber;
}

void Assembler::setCurrentCodeLineNumber(int newCurrentCodeLineNumber)
{
    if (currentFile.lineNumber == newCurrentCodeLineNumber)
        return;
    currentFile.lineNumber = newCurrentCodeLineNumber;
    emit currentCodeLineNumberChanged(currentFile.filename, currentFile.lineNumber);
}

const QList<Instruction> *Assembler::instructions() const
{
    return _instructions;
}

void Assembler::setInstructions(QList<Instruction> *newInstructions)
{
    _instructions = newInstructions;
}

const QList<Assembler::CodeFileLineNumber> &Assembler::instructionsCodeFileLineNumbers() const
{
    return _instructionsCodeFileLineNumbers;
}

int Assembler::currentCodeInstructionNumber() const
{
    return _currentCodeInstructionNumber;
}

void Assembler::setCurrentCodeInstructionNumber(int newCurrentCodeInstructionNumber)
{
    _currentCodeInstructionNumber = newCurrentCodeInstructionNumber;
}

void Assembler::setCodeLabel(const QString &key, int value)
{
    _codeLabels[key] = value;
}

void Assembler::setCode(QTextStream *codeStream)
{
    currentFile.stream = codeStream;
    restart();
}


void Assembler::debugMessage(const QString &message) const
{
    QString message2(message);
    QString filename;
    if (!currentFile.filename.isEmpty())
        filename = QString("\"%1\", ").arg(currentFile.filename);
    QString location;
    if (!filename.isEmpty() || currentFile.lineNumber >= 0)
        location = QString("[%1line #%2]").arg(filename).arg(currentFile.lineNumber + 1);
    if (!location.isEmpty())
        message2 = location + " " + message2;
    qDebug() << message2;
    emit sendMessageToConsole(message2);
}

void Assembler::assemblerWarning(const QString &message) const
{
    debugMessage("Assembler Warning: " + message);
}

void Assembler::assemblerError(const QString &message) const
{
    debugMessage("Assembler Error: " + message);
}


QString Assembler::scopedLabelName(const QString &label) const
{
    if (!label.startsWith('.'))
        return label;
    if (_currentCodeLabelScope.isEmpty())
        assemblerWarning(QString("Local label not in any other label scope: %1").arg(label));
    return _currentCodeLabelScope + label;
}

void Assembler::assignLabelValue(const QString &scopedLabel, int value)
{
    if (scopedLabel.isEmpty())
        assemblerWarning(QString("Defining label with no name"));
    if (_codeLabels.value(scopedLabel, value) != value)
        assemblerWarning(QString("Label redefinition: %1").arg(scopedLabel));
    setCodeLabel(scopedLabel, value);
    if (!scopedLabel.contains('.'))
        _currentCodeLabelScope = scopedLabel;
}


void Assembler::restart(bool assemblePass2 /*= false*/)
{
    Q_ASSERT(currentFile.stream);

    if (!assemblePass2)
    {
        while (!codeFileStateStack.isEmpty())
        {
            closeIncludeFile();
            CodeFileState state = codeFileStateStack.pop();
            currentFile.stream = state.stream;
            Q_ASSERT(currentFile.stream);
        }
        currentFile.filename.clear();
        currentFile.file = nullptr;
        currentFile.lines.clear();
        currentFile.stream->seek(0);
        _codeLabels.clear();
        _codeLabels["__outch"] = InternalJSRs::__JSR_outch;
        _codeLabels["__get_time"] = InternalJSRs::__JSR_get_time;
        _codeLabels["__get_elapsed_time"] = InternalJSRs::__JSR_get_elapsed_time;
        setAssembleState(AssembleState::NotStarted);
    }

    setCurrentCodeLineNumber(0);
    setCurrentCodeInstructionNumber(0);
    currentToken.clear();
    _currentCodeLabelScope = "";
}

void Assembler::assemble()
{
    restart();
    setAssembleState(Pass1);
    if (!assemblePass())
        return;
    restart(true);
    setAssembleState(Pass2);
    if (!assemblePass())
        return;
    setAssembleState(Assembled);
}


bool Assembler::assemblePass()
{
    _instructions->clear();
    _instructionsCodeFileLineNumbers.clear();
    Opcodes opcode;
    OpcodeOperand operand;
    bool hasOpcode, eof;
    while (assembleNextStatement(opcode, operand, hasOpcode, eof))
    {
        if (hasOpcode)
        {
            _instructions->append(Instruction(opcode, operand));
            _instructionsCodeFileLineNumbers.append(CodeFileLineNumber(currentFile.filename, currentFile.lineNumber));
            setCurrentCodeInstructionNumber(_currentCodeInstructionNumber + 1);
        }
        setCurrentCodeLineNumber(currentFile.lineNumber + 1);
    }
    if (!eof)
        return false;
    return true;
}

bool Assembler::assembleNextStatement(Opcodes &opcode, OpcodeOperand &operand, bool &hasOpcode, bool &eof)
{
    //
    // ASSEMBLING PHASE
    //

    hasOpcode = eof = false;
    currentToken.clear();
    if (!getNextLine())
    {
        eof = true;
        return false;
    }
    bool blankLine = !getNextToken();
    if (blankLine)
        return true;

    QString mnemonic(currentToken);
    opcode = Assembly::OpcodesKeyToValue(mnemonic.toUpper().toLocal8Bit());
    hasOpcode = Assembly::OpcodesValueIsValid(opcode);
    if (!hasOpcode)
    {
        if (tokenIsLabel())
        {
            QString label(currentToken);

            bool more = getNextToken();

            if (currentToken == '=')
            {
                getNextToken();
                bool ok;
                int value = getTokensExpressionValueAsInt(ok);
                if (ok)
                    assignLabelValue(scopedLabelName(label), value);
                return true;
            }
            else
            {
                bool isCodeLabel = true;
                if (_codeLabelRequiresColon)
                {
                    if (currentToken == ':')
                        more = getNextToken();
                    else
                    {
                        isCodeLabel = false;
                        more = true;
                        currentToken = label;
                    }
                }
                if (isCodeLabel)
                    assignLabelValue(scopedLabelName(label), _currentCodeInstructionNumber);
            }

            if (!more)
                return true;

            mnemonic = currentToken;
            opcode = Assembly::OpcodesKeyToValue(mnemonic.toUpper().toLocal8Bit());
            hasOpcode = Assembly::OpcodesValueIsValid(opcode);
        }

        if (tokenIsDirective())
        {
            QString directive(currentToken);

            if (directive == ".byte")
            {
                getNextToken();
                bool ok;
                int value = getTokensExpressionValueAsInt(ok);
                if (!ok)
                {
                    assemblerError(QString("Bad value: %1").arg(currentToken));
                    return false;
                }
                Q_UNUSED(value)
                assemblerError(QString("Cannot yet implement directive: %1").arg(directive));
                return false;
            }
            if (directive == ".include")
            {
                getNextToken();
                bool ok = currentToken.length() >= 2 && currentToken.at(0) == '\"' && currentToken.at(currentToken.size() - 1) == '\"';
                if (!ok)
                {
                    assemblerError(QString("Bad value: %1").arg(currentToken));
                    return false;
                }
                QString value = currentToken.mid(1, currentToken.size() - 2);
                return startIncludeFile(value);
            }
            else
            {
                assemblerError(QString("Unimplemented directive: %1").arg(directive));
                return false;
            }
        }
    }

    if (!hasOpcode)
    {
        assemblerError(QString("Unrecognized opcode mnemonic: %1").arg(mnemonic));
        return false;
    }

    QString opcodeName(Assembly::OpcodesValueToString(opcode));
    getNextToken();
    if (currentToken.isEmpty())
    {
        operand.mode = AddressingMode::Implicit;
        operand.arg = 0;
    }
    else if (currentToken == '#')
    {
        operand.mode = AddressingMode::Immediate;
        getNextToken();
        bool ok;
        int value = getTokensExpressionValueAsInt(ok);
        if (!ok)
        {
            assemblerError(QString("Bad value: %1").arg(currentToken));
            return false;
        }
        if (value < -128 || value > 256)
            assemblerWarning(QString("Value out of range for opcode: $%1 %2").arg(value, 2, 16, QChar('0')).arg(opcodeName));
        operand.arg = value;
    }
    else if (currentToken.toUpper() == "A")
    {
        operand.mode = AddressingMode::Accumulator;
        operand.arg = 0;
        getNextToken();
    }
    else if (tokenIsInt() || tokenIsLabel() || currentToken == "*" || currentToken == "(")
    {
        QString firstToken(currentToken);
        bool allowForIndirectAddressing = firstToken == "(";
        const Assembly::OpcodesInfo &opcodeInfo(Assembly::getOpcodeInfo(opcode));
        operand.mode = AddressingMode::Absolute;
        if (opcodeInfo.modes & AddressingMode::Relative)
            operand.mode = AddressingMode::Relative;
        else if (allowForIndirectAddressing)
            if (opcodeInfo.modes & (AddressingMode::Indirect | AddressingMode::IndexedIndirectX | AddressingMode::IndirectIndexedY))
                operand.mode = AddressingMode::Indirect;

        bool ok;
        bool indirectAddressingMetCloseParen;
        int value = getTokensExpressionValueAsInt(ok, allowForIndirectAddressing, &indirectAddressingMetCloseParen);
        if (!ok)
        {
            assemblerError(QString("Bad value: %1").arg(currentToken));
            return false;
        }
        operand.arg = value;

        if (!currentToken.isEmpty())
        {
            bool recognised = false;
            if (currentToken == ",")
            {
                getNextToken();
                if (currentToken.toUpper() == "X")
                {
                    operand.mode = AddressingMode::AbsoluteX;
                    recognised = true;
                    if (allowForIndirectAddressing)
                    {
                        if (!indirectAddressingMetCloseParen)
                        {
                            if (getNextToken() && currentToken == ")")
                                operand.mode = AddressingMode::IndexedIndirectX;
                        }
                        else
                            recognised = false;
                    }
                }
                else if (currentToken.toUpper() == "Y")
                {
                    operand.mode = AddressingMode::AbsoluteY;
                    recognised = true;
                    if (allowForIndirectAddressing)
                    {
                        if (indirectAddressingMetCloseParen)
                            operand.mode = AddressingMode::IndirectIndexedY;
                        else
                            recognised = false;
                    }
                }
            }
            if (!recognised)
            {
                assemblerError(QString("Unrecognized operand addressing mode for opcode: %1 %2").arg(currentToken).arg(opcodeName));
                return false;
            }
        }
    }
    else
    {
        assemblerError(QString("Unrecognized operand addressing mode for opcode: %1 %2").arg(currentToken).arg(opcodeName));
        return false;
    }

    bool zpArg = (operand.arg & 0xff00) == 0;
    const Assembly::OpcodesInfo &opcodeInfo(Assembly::getOpcodeInfo(opcode));
    switch (operand.mode)
    {
    case AddressingMode::Absolute:
        if (zpArg && (opcodeInfo.modes & AddressingMode::ZeroPage))
            operand.mode = AddressingMode::ZeroPage;
        break;
    case AddressingMode::AbsoluteX:
        if (zpArg && (opcodeInfo.modes & AddressingMode::ZeroPageX))
            operand.mode = AddressingMode::ZeroPageX;
        break;
    case AddressingMode::AbsoluteY:
        if (zpArg && (opcodeInfo.modes & AddressingMode::ZeroPageY))
            operand.mode = AddressingMode::ZeroPageY;
        break;
    case AddressingMode::IndexedIndirectX:
    case AddressingMode::IndirectIndexedY:
        if (!zpArg)
        {
            assemblerError(QString("Operand argument value must be ZeroPage: %1 %2").arg(Assembly::AddressingModeValueToString(operand.mode)).arg(opcodeName));
            return false;
        }
        break;
    case AddressingMode::Relative: {
        int relative = operand.arg - _currentCodeInstructionNumber;
        if (relative < -128 || relative > 127)
            if (_assembleState == Pass2)
            {
                assemblerError(QString("Relative mode branch address out of range: %1 %2").arg(relative).arg(opcodeName));
                return false;
            }
        operand.arg = relative;
        break;
    }
    default: break;
    }

    if (getNextToken())
    {
        assemblerError(QString("Unexpected token at end of statement: %1 %2").arg(currentToken).arg(opcodeName));
        return false;
    }

    if (!Assembly::opcodeSupportsAddressingMode(opcode, operand.mode))
    {
        assemblerError(QString("Opcode does not support operand addressing mode: %1 %2")
                         .arg(Assembly::OpcodesValueToString(opcode))
                         .arg(Assembly::AddressingModeValueToString(operand.mode)));
        return false;
    }

    return true;
}


const QStringList &Assembler::codeIncludeDirectories() const
{
    return _codeIncludeDirectories;
}

void Assembler::setCodeIncludeDirectories(const QStringList &newCodeIncludeDirectories)
{
    _codeIncludeDirectories = newCodeIncludeDirectories;
}

QString Assembler::findIncludeFilePath(const QString &includeFilename)
{
    if (includeFilename.isEmpty())
        return includeFilename;
    QFileInfo fileInfo(includeFilename);
    if (!fileInfo.isRelative())
        return includeFilename;
    if (!currentFile.filename.isEmpty())
    {
        fileInfo.setFile(QDir(fileInfo.dir()).filePath(includeFilename));
        if (fileInfo.exists())
            return fileInfo.filePath();
    }
    for (const QString &includeDir : _codeIncludeDirectories)
    {
        fileInfo.setFile(QDir(includeDir).filePath(includeFilename));
        if (fileInfo.exists())
            return fileInfo.filePath();
    }
    return includeFilename;
}

bool Assembler::startIncludeFile(const QString &includeFilename)
{
    QString includeFilePath = findIncludeFilePath(includeFilename);
    QFile *file = new QFile(includeFilePath);
    if (!file->open(QFile::ReadOnly | QFile::Text))
    {
        assemblerError(QString("Could not include file: %1: %2").arg(includeFilePath).arg(file->errorString()));
        delete file;
        return false;
    }

    CodeFileState state(currentFile);

    codeFileStateStack.push(state);

    currentFile.filename = includeFilename;
    currentFile.lineNumber = -1;
    currentFile.file = file;
    currentFile.stream = new QTextStream(file);
    currentFile.lines = QStringList();

    return true;
}

void Assembler::endIncludeFile()
{
    if (codeFileStateStack.isEmpty())
        return;

    closeIncludeFile();

    currentFile = codeFileStateStack.pop();
}

void Assembler::closeIncludeFile()
{
    if (currentFile.file != nullptr)
    {
        if (currentFile.stream != nullptr)
        {
            delete currentFile.stream;
            currentFile.stream = nullptr;
        }
        currentFile.file->close();
        delete currentFile.file;
        currentFile.file = nullptr;
    }
}


bool Assembler::getNextLine()
{
    currentLine.line.clear();
    while (currentFile.lineNumber >= currentFile.lines.size())
    {
        if (currentFile.stream->atEnd() && !codeFileStateStack.isEmpty())
        {
            endIncludeFile();
            setCurrentCodeLineNumber(currentFile.lineNumber + 1);
            continue;
        }
        if (!currentFile.stream->readLineInto(&currentLine.line))
            return false;
        currentFile.lines.append(currentLine.line);
    }
    if (currentFile.lineNumber >= currentFile.lines.size())
        return false;
    currentLine.line = currentFile.lines.at(currentFile.lineNumber);
    currentLine.lineStream.setString(&currentLine.line, QIODeviceBase::ReadOnly);
    return true;
}

bool Assembler::getNextToken(bool wantOperator /*= false*/)
{
    currentToken.clear();
    QChar firstChar, nextChar;

    QTextStream &currentLineStream(currentLine.lineStream);
    currentLineStream >> firstChar;
    while (firstChar.isSpace())
        currentLineStream >> firstChar;

    if (firstChar.isNull())
        return false;
    if (firstChar == ';')
    {
        QString comment = currentLineStream.readLine();
        Q_UNUSED(comment)
        return false;
    }

    currentToken.append(firstChar);
    if (firstChar.isPunct() || firstChar.isSymbol())
        if (!(firstChar == '%' || firstChar == '$' || firstChar == '\'' || firstChar == '\"' || firstChar == '_' || firstChar == '.'
              || (!wantOperator && (firstChar == '-' || firstChar == '*'))
              || (wantOperator && (firstChar == '<' || firstChar == '>'))
              ))
            return true;
    currentLineStream >> nextChar;
    if (firstChar == '\'')
    {
        if (!nextChar.isNull())
        {
            currentToken.append(nextChar);
            currentLineStream >> nextChar;
            if (nextChar == '\'')
            {
                currentToken.append(nextChar);
                currentLineStream >> nextChar;
            }
        }
    }
    else if (firstChar == '\"')
    {
        while (!(nextChar.isNull() || nextChar == '\"'))
        {
            currentToken.append(nextChar);
            currentLineStream >> nextChar;
        }
        if (nextChar == '\"')
        {
            currentToken.append(nextChar);
            currentLineStream >> nextChar;
        }
    }
    else if (firstChar == '<' || firstChar == '>')
    {
        if (nextChar == firstChar)
        {
            currentToken.append(nextChar);
            currentLineStream >> nextChar;
        }
    }
    else
    {
        while (!(nextChar.isNull() || nextChar.isSpace() || (nextChar.isPunct() && nextChar != '_') || nextChar.isSymbol()))
        {
            currentToken.append(nextChar);
            currentLineStream >> nextChar;
        }
    }
    if (!nextChar.isNull())
        currentLineStream.seek(currentLineStream.pos() - 1);
    return true;
}

int Assembler::getTokensExpressionValueAsInt(bool &ok, bool allowForIndirectAddressing /*= false*/, bool *indirectAddressingMetCloseParen /*= nullptr*/)
{
    enum Operators { OpenParen, CloseParen, BitOr, BitAnd, ShiftLeft, ShiftRight, Plus, Minus, Multiply, Divide, };
    struct OperatorInfo { Operators _operator; int precedence; QString string; };
    static const OperatorInfo operatorsInfo[]
    {
        { OpenParen, 0, "(", },
        { CloseParen, 0, ")", },
        { BitOr, 10, "|", },
        { BitAnd, 20, "&", },
        { ShiftLeft, 30, "<<", },
        { ShiftRight, 30, ">>", },
        { Plus, 40, "+", },
        { Minus, 40, "-", },
        { Multiply, 50, "*", },
        { Divide, 50, "/", },
    };
    static_assert(OpenParen == 0, "Operators::OpenParen must have a value of 0");
    QStack<int> operatorStack;
    QStack<int> valueStack;

    QString firstToken(currentToken);
    if (firstToken != "(")
        allowForIndirectAddressing = false;
    ok = false;
    if (indirectAddressingMetCloseParen != nullptr)
        *indirectAddressingMetCloseParen = false;
    int value = -1;
    bool wantOperator = false;
    bool endOfExpression = false;
    do
    {
        int foundOperator = -1;
        if (currentToken.isEmpty())
            endOfExpression = true;
        else if (currentToken == "," && allowForIndirectAddressing)
            endOfExpression = true;
        else if (!wantOperator)
        {
            if (currentToken == "(")
                operatorStack.push(OpenParen);
            else
            {
                value = tokenValueAsInt(&ok);
                if (!ok)
                    return -1;
                valueStack.push(value);
                wantOperator = true;
            }
        }
        else
        {
            QString _operator(currentToken);
            for (int i = 0; i < sizeof(operatorsInfo) / sizeof(operatorsInfo[0]) && foundOperator < 0; i++)
                if (operatorsInfo[i].string == _operator)
                    foundOperator = i;
            if (foundOperator < 0)
                endOfExpression = true;
            else if (operatorsInfo[foundOperator]._operator != CloseParen)
                wantOperator = false;
       }

       if (foundOperator >= 0 || endOfExpression)
       {
            while (!operatorStack.isEmpty() && operatorsInfo[operatorStack.top()]._operator != OpenParen
                   && (endOfExpression || operatorsInfo[foundOperator]._operator == CloseParen || operatorsInfo[operatorStack.top()].precedence >= operatorsInfo[foundOperator].precedence))
            {
                if (operatorStack.size() < 1 || valueStack.size() < 2)
                {
                   ok = false;
                   return -1;
                }
                int opIndex = operatorStack.pop();
                int valRight = valueStack.pop();
                int valLeft = valueStack.pop();
                switch (operatorsInfo[opIndex]._operator)
                {
                case BitOr:         value = valLeft | valRight; break;
                case BitAnd:        value = valLeft & valRight; break;
                case ShiftLeft:     value = valLeft << valRight; break;
                case ShiftRight:    value = valLeft >> valRight; break;
                case Plus:          value = valLeft + valRight; break;
                case Minus:         value = valLeft - valRight; break;
                case Multiply:      value = valLeft * valRight; break;
                case Divide:
                    if (valRight == 0)
                    {
                        assemblerWarning(QString("Division by zero"));
                        ok = false;
                        return -1;
                    }
                    value = valLeft / valRight;
                    break;
                default: Q_ASSERT(false); break;
                }
                valueStack.push(value);
            }

            if (foundOperator >= 0)
            {
                if (operatorsInfo[foundOperator]._operator == CloseParen)
                {
                    if (operatorStack.isEmpty() || operatorsInfo[operatorStack.top()]._operator != OpenParen)
                    {
                        ok = false;
                        return -1;
                    }
                    operatorStack.pop();
                    if (allowForIndirectAddressing)
                        if (operatorStack.isEmpty())
                        {
                            if (indirectAddressingMetCloseParen != nullptr)
                                *indirectAddressingMetCloseParen = true;
                            allowForIndirectAddressing = false;
                        }
                }
                else
                    operatorStack.push(foundOperator);
            }
        }

       if (!endOfExpression)
            getNextToken(wantOperator);

    } while (!endOfExpression);

    ok = false;
    if (valueStack.size() != 1)
        return -1;
    if (allowForIndirectAddressing)
        if (currentToken == "," && operatorStack.size() == 1 && operatorsInfo[operatorStack.top()]._operator == OpenParen)
            operatorStack.pop();
    if (!operatorStack.isEmpty())
        return -1;
    ok = true;
    return value;
}

bool Assembler::tokenIsDirective() const
{
    if (!currentToken.startsWith('.'))
        return false;
    return Assembly::directives().contains(currentToken);
}

bool Assembler::tokenIsLabel() const
{
    if (currentToken.size() == 0)
        return false;
    QChar ch = currentToken.at(0);
    if (!(ch == '.' || ch == '_' || ch.isLetter()))
        return false;
    if (ch == '.')
        if (tokenIsDirective())
            return false;
    for (int i = 1; i < currentToken.size(); i++)
    {
        ch = currentToken.at(i);
        if (!(ch == '_' || ch.isLetter() || ch.isDigit()))
            return false;
    }
    return true;
}

bool Assembler::tokenIsInt() const
{
    QChar firstChar = currentToken.size() > 0 ? currentToken.at(0) : QChar();
    return firstChar == '%' || firstChar == '$' || firstChar.isDigit() || firstChar == '-' || firstChar == '\'';
}

int Assembler::tokenToInt(bool *ok) const
{
    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    QChar firstChar = currentToken.size() > 0 ? currentToken.at(0) : QChar();
    if (firstChar == '%')
        return currentToken.mid(1).toInt(ok, 2);
    else if (firstChar == '$')
        return currentToken.mid(1).toInt(ok, 16);
    else if (firstChar.isDigit() || firstChar == '-')
        return currentToken.toInt(ok, 10);
    else if (firstChar == '\'')
    {
        QChar nextChar = currentToken.size() > 1 ? currentToken.at(1) : QChar();
        QChar nextChar2 = currentToken.size() > 2 ? currentToken.at(2) : QChar();
        if (nextChar2 == '\'')
        {
            *ok = true;
            return nextChar.toLatin1();
        }
    }
    *ok = false;
    return -1;
}

int Assembler::tokenValueAsInt(bool *ok) const
{
    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    int value;
    value = tokenToInt(ok);
    if (*ok)
        return value;
    if (tokenIsLabel())
    {
        QString label = scopedLabelName(currentToken);
        if (_codeLabels.contains(label))
        {
            *ok = true;
            return _codeLabels.value(label);
        }
        else if (_assembleState == Pass1)
        {
            *ok = true;
            return 0xffff;
        }
        sendMessageToConsole(QString("Label not defined: %1").arg(currentToken));
    }
    else if (currentToken == "*")
    {
        *ok = true;
        return _currentCodeInstructionNumber;
    }
    *ok = false;
    return -1;
}

