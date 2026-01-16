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

uint8_t *Assembler::memory() const
{
    return _memory;
}

void Assembler::setMemory(uint8_t *newMemory)
{
    _memory = newMemory;
}

const uint16_t Assembler::defaultLocationCounter() const
{
    return _defaultLocationCounter;
}

const Instruction *Assembler::instructions() const
{
    return _instructions;
}

void Assembler::setInstructions(Instruction *newInstructions)
{
    _instructions = newInstructions;
}

const QList<Assembler::CodeFileLineNumber> &Assembler::instructionsCodeFileLineNumbers() const
{
    return _instructionsCodeFileLineNumbers;
}

uint16_t Assembler::locationCounter() const
{
    return _locationCounter;
}

void Assembler::setLocationCounter(uint16_t newLocationCounter)
{
    _locationCounter = newLocationCounter;
}

QList<uint16_t> *Assembler::breakpoints() const
{
    return _breakpoints;
}

void Assembler::setBreakpoints(QList<uint16_t> *newBreakpoints)
{
    _breakpoints = newBreakpoints;
}

int Assembler::codeLabelValue(const QString &key) const
{
    return _codeLabels.value(key, -1);
}


void Assembler::setCodeLabelValue(const QString &key, int value)
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

void Assembler::assemblerWarningMessage(const QString &message) const
{
    debugMessage("Assembler Warning: " + message);
}

void Assembler::assemblerErrorMessage(const QString &message) const
{
    debugMessage("Assembler Error: " + message);
}


QString Assembler::scopedLabelName(const QString &label) const
{
    if (!label.startsWith('.'))
        return label;
    if (_currentCodeLabelScope.isEmpty())
        assemblerWarningMessage(QString("Local label not in any other label scope: %1").arg(label));
    return _currentCodeLabelScope + label;
}

void Assembler::assignLabelValue(const QString &scopedLabel, int value)
{
    if (scopedLabel.isEmpty())
        assemblerWarningMessage(QString("Defining label with no name"));
    if (_codeLabels.value(scopedLabel, value) != value)
        assemblerWarningMessage(QString("Label redefinition: %1").arg(scopedLabel));
    setCodeLabelValue(scopedLabel, value);
    if (!scopedLabel.contains('.'))
        _currentCodeLabelScope = scopedLabel;
}

void Assembler::cleanup()
{
    Q_ASSERT(currentFile.stream);

    while (!codeFileStateStack.isEmpty())
    {
        closeIncludeFile();
        CodeFileState state = codeFileStateStack.pop();
        currentFile.stream = state.stream;
        Q_ASSERT(currentFile.stream);
    }
    currentFile.filename.clear();
    currentFile.file = nullptr;
    currentFile.stream->seek(0);
    currentFile.lines.clear();
    _breakpoints->clear();
    _codeLabels.clear();
    _codeLabels["__terminate"] = InternalJSRs::__JSR_terminate;
    _codeLabels["__brk_handler"] = InternalJSRs::__JSR_brk_handler;
    _codeLabels["__outch"] = InternalJSRs::__JSR_outch;
    _codeLabels["__get_time"] = InternalJSRs::__JSR_get_time;
    _codeLabels["__get_elapsed_time"] = InternalJSRs::__JSR_get_elapsed_time;
    _currentCodeLabelScope = "";
    currentToken.clear();
    setAssembleState(AssembleState::NotStarted);
}


void Assembler::restart(bool assemblePass2 /*= false*/)
{
    Q_ASSERT(currentFile.stream);

    if (!assemblePass2)
        cleanup();

    _currentCodeLabelScope = "";
    currentToken.clear();
    setCurrentCodeLineNumber(0);
    setLocationCounter(_defaultLocationCounter);
}

void Assembler::assemble()
{
    try
    {
        restart();
        setAssembleState(Pass1);
        assemblePass();
        restart(true);
        setAssembleState(Pass2);
        assemblePass();
        setAssembleState(Assembled);
    }
    catch (const AssemblerError &e)
    {
        assemblerErrorMessage(QString(e.what()));
        cleanup();
        setAssembleState(NotStarted);
    }
}


void Assembler::assemblePass()
{
    setLocationCounter(_defaultLocationCounter);
    _instructionsCodeFileLineNumbers.clear();
    Operation operation;
    AddressingMode mode;
    uint16_t arg;
    bool hasOperation, eof;
    assembleNextStatement(operation, mode, arg, hasOperation, eof);
    while (!eof)
    {
        if (hasOperation)
        {
            const InstructionInfo *instructionInfo(Assembly::findInstructionInfo(operation, mode));
            if (instructionInfo == nullptr)
                throw AssemblerError(QString("Operation does not support operand addressing mode: %1 %2")
                                         .arg(Assembly::OperationValueToString(operation))
                                         .arg(Assembly::AddressingModeValueToString(mode)));
            uint8_t bytes = instructionInfo->bytes;
            Q_ASSERT(_locationCounter + bytes <= 0x10000);
            int i = 0;
            while (i < _instructionsCodeFileLineNumbers.size() && _instructionsCodeFileLineNumbers.at(i)._locationCounter <= _locationCounter)
                i++;
            _instructionsCodeFileLineNumbers.insert(i, CodeFileLineNumber(_locationCounter, currentFile.filename, currentFile.lineNumber));
            uint8_t *address = reinterpret_cast<uint8_t *>(_instructions) + _locationCounter;
            Instruction *instruction = reinterpret_cast<Instruction *>(address);
            *instruction = Instruction(instructionInfo->opcodeByte, arg);
            address[0] = instructionInfo->opcodeByte;
            if (bytes > 1)
            {
                address[1] = static_cast<uint8_t>(arg);
                if (bytes > 2)
                    address[2] = static_cast<uint8_t>(arg >> 8);
            }
            setLocationCounter(_locationCounter + bytes);
        }
        setCurrentCodeLineNumber(currentFile.lineNumber + 1);
        assembleNextStatement(operation, mode, arg, hasOperation, eof);
    }
}

void Assembler::assembleNextStatement(Operation &operation, AddressingMode &mode, uint16_t &arg, bool &hasOperation, bool &eof)
{
    //
    // ASSEMBLING PHASE
    //

    hasOperation = eof = false;
    currentToken.clear();
    if (!getNextLine())
    {
        eof = true;
        return;
    }
    bool blankLine = !getNextToken();
    if (blankLine)
        return;

    QString mnemonic(currentToken);
    operation = Assembly::OperationKeyToValue(mnemonic.toUpper().toLocal8Bit());
    hasOperation = Assembly::OperationValueIsValid(operation);
    if (!hasOperation)
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
                if (!ok)
                    throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                assignLabelValue(scopedLabelName(label), value);
                return;
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
                    assignLabelValue(scopedLabelName(label), _locationCounter);
            }

            if (!more)
                return;

            mnemonic = currentToken;
            operation = Assembly::OperationKeyToValue(mnemonic.toUpper().toLocal8Bit());
            hasOperation = Assembly::OperationValueIsValid(operation);
        }

        if (tokenIsDirective())
        {
            QString directive(currentToken);

            if (directive == ".byte")
            {
                do
                {
                    uint8_t *address = reinterpret_cast<uint8_t *>(_instructions) + _locationCounter;
                    getNextToken();
                    bool ok;
                    if (tokenIsString())
                    {
                        QString strValue = tokenToString(&ok);
                        if (!ok)
                            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                        int len = strValue.length();
                        std::memcpy(address, strValue.toUtf8().constData(), len);
                        setLocationCounter(_locationCounter + len);
                        getNextToken();
                    }
                    else
                    {
                        int intValue = getTokensExpressionValueAsInt(ok);
                        if (!ok)
                            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                        *address = intValue & 0xff;
                        setLocationCounter(_locationCounter + 1);
                    }
                } while (currentToken == ",");
                return;
            }
            else if (directive == ".include")
            {
                getNextToken();
                bool ok;
                QString value = tokenToString(&ok);
                if (!ok)
                    throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                startIncludeFile(value);
                return;
            }
            else if (directive == ".break")
            {
                if (!_breakpoints->contains(_locationCounter))
                    _breakpoints->append(_locationCounter);
                return;
            }
            else if (directive == ".org")
            {
                getNextToken();
                bool ok;
                int value = getTokensExpressionValueAsInt(ok);
                if (!ok)
                    throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                if (value < 0 || value > 0xffff)
                    assemblerWarningMessage(QString("Value out of range for directive: $%1 %2").arg(value, 2, 16, QChar('0')).arg(directive));
                setLocationCounter(value);
                return;
            }
            else
                throw AssemblerError(QString("Unimplemented directive: %1").arg(directive));
        }
    }

    if (!hasOperation)
        throw AssemblerError(QString("Unrecognized operation mnemonic: %1").arg(mnemonic));

    QString operationName(Assembly::OperationValueToString(operation));
    getNextToken();
    if (currentToken.isEmpty())
    {
        mode = AddressingMode::Implied;
        arg = 0;
    }
    else if (currentToken == '#')
    {
        mode = AddressingMode::Immediate;
        getNextToken();
        bool ok;
        int value = getTokensExpressionValueAsInt(ok);
        if (!ok)
            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
        if (value < -128 || value > 255)
            assemblerWarningMessage(QString("Value out of range for operation: $%1 %2").arg(value, 2, 16, QChar('0')).arg(operationName));
        arg = value;
    }
    else if (currentToken.toUpper() == "A")
    {
        mode = AddressingMode::Accumulator;
        arg = 0;
        getNextToken();
    }
    else if (tokenIsInt() || tokenIsLabel() || currentToken == "*" || currentToken == "(")
    {
        QString firstToken(currentToken);
        bool allowForIndirectAddressing = firstToken == "(";
        const Assembly::OperationInfo &operationInfo(Assembly::getOperationInfo(operation));
        mode = AddressingMode::Absolute;
        if (operationInfo.modes & AddressingModeFlag::RelativeFlag)
            mode = AddressingMode::Relative;
        else if (allowForIndirectAddressing)
            if (operationInfo.modes & (AddressingModeFlag::IndirectFlag | AddressingModeFlag::IndexedIndirectXFlag | AddressingModeFlag::IndirectIndexedYFlag))
                mode = AddressingMode::Indirect;

        bool ok;
        bool indirectAddressingMetCloseParen;
        int value = getTokensExpressionValueAsInt(ok, allowForIndirectAddressing, &indirectAddressingMetCloseParen);
        if (!ok)
            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
        if (value < -32768 || value > 65535)
            assemblerWarningMessage(QString("Value out of range for operation: $%1 %2").arg(value, 2, 16, QChar('0')).arg(operationName));
        arg = value;

        if (!currentToken.isEmpty())
        {
            bool recognised = false;
            if (currentToken == ",")
            {
                getNextToken();
                if (currentToken.toUpper() == "X")
                {
                    mode = AddressingMode::AbsoluteX;
                    recognised = true;
                    if (allowForIndirectAddressing)
                    {
                        if (!indirectAddressingMetCloseParen)
                        {
                            if (getNextToken() && currentToken == ")")
                                mode = AddressingMode::IndexedIndirectX;
                        }
                        else
                            recognised = false;
                    }
                }
                else if (currentToken.toUpper() == "Y")
                {
                    mode = AddressingMode::AbsoluteY;
                    recognised = true;
                    if (allowForIndirectAddressing)
                    {
                        if (indirectAddressingMetCloseParen)
                            mode = AddressingMode::IndirectIndexedY;
                        else
                            recognised = false;
                    }
                }
            }
            if (!recognised)
                throw AssemblerError(QString("Unrecognized operand addressing mode for operation: %1 %2").arg(currentToken).arg(operationName));
        }
    }
    else
        throw AssemblerError(QString("Unrecognized operand addressing mode for operation: %1 %2").arg(currentToken).arg(operationName));

    bool zpArg = (arg & 0xff00) == 0;
    const Assembly::OperationInfo &operationInfo(Assembly::getOperationInfo(operation));
    switch (mode)
    {
    case AddressingMode::Absolute:
        if (zpArg && (operationInfo.modes & AddressingModeFlag::ZeroPageFlag))
            mode = AddressingMode::ZeroPage;
        break;
    case AddressingMode::AbsoluteX:
        if (zpArg && (operationInfo.modes & AddressingModeFlag::ZeroPageXFlag))
            mode = AddressingMode::ZeroPageX;
        break;
    case AddressingMode::AbsoluteY:
        if (zpArg && (operationInfo.modes & AddressingModeFlag::ZeroPageYFlag))
            mode = AddressingMode::ZeroPageY;
        break;
    case AddressingMode::IndexedIndirectX:
    case AddressingMode::IndirectIndexedY:
        if (!zpArg)
            throw AssemblerError(QString("Operand argument value must be ZeroPage: %1 %2").arg(Assembly::AddressingModeValueToString(mode)).arg(operationName));
        break;
    case AddressingMode::Relative: {
        int relative = arg - _locationCounter;
        if (relative < -128 || relative > 127)
            if (_assembleState == Pass2)
                throw AssemblerError(QString("Relative mode branch address out of range: %1 %2").arg(relative).arg(operationName));
        arg = relative;
        break;
    }
    default: break;
    }

    if (getNextToken())
        throw AssemblerError(QString("Unexpected token at end of statement: %1 %2").arg(currentToken).arg(operationName));

    if (!Assembly::operationSupportsAddressingMode(operation, mode))
        throw AssemblerError(QString("Operation does not support operand addressing mode: %1 %2")
                         .arg(Assembly::OperationValueToString(operation))
                         .arg(Assembly::AddressingModeValueToString(mode)));
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

void Assembler::startIncludeFile(const QString &includeFilename)
{
    QString includeFilePath = findIncludeFilePath(includeFilename);
    QFile *file = new QFile(includeFilePath);
    if (!file->open(QFile::ReadOnly | QFile::Text))
    {
        QString errorString(file->errorString());
        delete file;
        throw AssemblerError(QString("Could not include file: %1: %2").arg(includeFilePath).arg(errorString));
    }

    CodeFileState state(currentFile);

    codeFileStateStack.push(state);

    currentFile.filename = includeFilename;
    currentFile.lineNumber = -1;
    currentFile.file = file;
    currentFile.stream = new QTextStream(file);
    currentFile.lines = QStringList();
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
                        assemblerWarningMessage(QString("Division by zero"));
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
        assemblerWarningMessage(QString("Label not defined: %1").arg(currentToken));
    }
    else if (currentToken == "*")
    {
        *ok = true;
        return _locationCounter;
    }
    *ok = false;
    return -1;
}

bool Assembler::tokenIsString() const
{
    return currentToken.size() > 0 ? currentToken.at(0) == '\"' : false;
}

QString Assembler::tokenToString(bool *ok) const
{
    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    if (currentToken.length() >= 2 && currentToken.at(0) == '\"' && currentToken.at(currentToken.size() - 1) == '\"')
    {
        *ok = true;
        return currentToken.mid(1, currentToken.size() - 2);
    }
    *ok = false;
    return QString();
}


//
// AssemblerError Class
//

AssemblerError::AssemblerError(const QString &msg)
    : std::runtime_error(msg.toStdString())
{

}
