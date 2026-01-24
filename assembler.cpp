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
    _includedFilePaths.clear();
    _assembleState = AssembleState::NotStarted;
    assemblerBreakpointProvider = nullptr;
}

void Assembler::setAssemblerBreakpointProvider(IAssemblerBreakpointProvider *provider)
{
    assemblerBreakpointProvider = provider;
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

void Assembler::cleanup(bool assemblePass2 /*= false*/)
{
    Q_ASSERT(currentFile.stream);

    while (!codeFileStateStack.isEmpty())
    {
        closeIncludeFile();
        CodeFileState state = codeFileStateStack.pop();
        currentFile.stream = state.stream;
        Q_ASSERT(currentFile.stream);
    }
    _includedFilePaths.clear();
    currentFile.filename.clear();
    currentFile.file = nullptr;
    currentFile.stream->seek(0);
    currentFile.lines.clear();
    _currentCodeLabelScope = "";
    currentToken.clear();
    if (assemblePass2)
        return;

    assemblerBreakpointProvider->clearBreakpoints();
    _codeLabels.clear();
    _codeLabels["__terminate"] = InternalJSRs::__JSR_terminate;
    _codeLabels["__brk_handler"] = InternalJSRs::__JSR_brk_handler;
    _codeLabels["__outch"] = InternalJSRs::__JSR_outch;
    _codeLabels["__get_time"] = InternalJSRs::__JSR_get_time;
    _codeLabels["__get_elapsed_time"] = InternalJSRs::__JSR_get_elapsed_time;
    _codeLabels["__clear_elapsed_time"] = InternalJSRs::__JSR_clear_elapsed_time;
    _codeLabels["__process_events"] = InternalJSRs::__JSR_process_events;
    _codeLabels["__inch"] = InternalJSRs::__JSR_inch;
    _codeLabels["__inkey"] = InternalJSRs::__JSR_inkey;
    _codeLabels["__wait"] = InternalJSRs::__JSR_wait;
    setAssembleState(AssembleState::NotStarted);
}

void Assembler::addInstructionsCodeFileLineNumber(const CodeFileLineNumber &cfln)
{
    int i = 0;
    while (i < _instructionsCodeFileLineNumbers.size() && _instructionsCodeFileLineNumbers.at(i)._locationCounter <= cfln._locationCounter)
        i++;
    _instructionsCodeFileLineNumbers.insert(i, cfln);
}


void Assembler::restart(bool assemblePass2 /*= false*/)
{
    Q_ASSERT(currentFile.stream);

    cleanup(assemblePass2);

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
            addInstructionsCodeFileLineNumber(CodeFileLineNumber(_locationCounter, currentFile.filename, currentFile.lineNumber));
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
    for (int i = 1; i < _instructionsCodeFileLineNumbers.size(); i++)
        Q_ASSERT(_instructionsCodeFileLineNumbers.at(i)._locationCounter > _instructionsCodeFileLineNumbers.at(i - 1)._locationCounter);
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
    assembleNextStatement(operation, mode, arg, hasOperation);
    if (!currentToken.isEmpty())
        throw AssemblerError(QString("Unexpected token at end of statement: %1").arg(currentToken));
}

void Assembler::assembleNextStatement(Operation &operation, AddressingMode &mode, uint16_t &arg, bool &hasOperation)
{
    //
    // ASSEMBLING PHASE
    //

    hasOperation = false;
    currentToken.clear();
    bool blankLine = !getNextToken();
    if (blankLine)
        return;

    QString mnemonic(currentToken);
    operation = Assembly::OperationKeyToValue(mnemonic.toUpper().toLatin1());
    hasOperation = Assembly::OperationValueIsValid(operation);
    if (!hasOperation)
    {
        if (tokenIsLabel())
        {
            QString label(currentToken);

            bool isCodeLabel = true;
            QString nextToken = peekNextToken();
            if (_codeLabelRequiresColon)
            {
                if (nextToken == ":")
                    getNextToken();
                else
                    isCodeLabel = false;
            }

            if (nextToken == "=")
            {
                getNextToken();
                getNextToken();
                bool ok;
                int value = getTokensExpressionValueAsInt(ok);
                if (!ok)
                    throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                assignLabelValue(scopedLabelName(label), value);
                return;
            }
            else if (isCodeLabel)
            {
                assignLabelValue(scopedLabelName(label), _locationCounter);
                getNextToken();
            }

            if (currentToken.isEmpty())
                return;
            mnemonic = currentToken;
            operation = Assembly::OperationKeyToValue(mnemonic.toUpper().toLatin1());
            hasOperation = Assembly::OperationValueIsValid(operation);
        }

        if (tokenIsDirective())
        {
            assembleDirective();
            return;
        }
    }

    if (!hasOperation)
        throw AssemblerError(QString("Unrecognized operation mnemonic: %1").arg(mnemonic));

    QString operationName(Assembly::OperationValueToString(operation));
    getNextToken();
    bool recognised = false;
    if (currentToken.isEmpty())
    {
        mode = AddressingMode::Implied;
        arg = 0;
        recognised = true;
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
        recognised = true;
    }
    else if (currentToken.toUpper() == "A")
    {
        mode = AddressingMode::Accumulator;
        arg = 0;
        getNextToken();
        recognised = true;
    }
    else if (tokenIsInt() || tokenIsLabel() || currentToken == "*" || currentToken == "(")
    {
        QString firstToken(currentToken);
        bool allowForIndirectAddressing = firstToken == "(";
        const Assembly::OperationMode &operationMode(Assembly::getOperationMode(operation));
        mode = AddressingMode::Absolute;
        if (operationMode.modes.testFlag(AddressingModeFlag::RelativeFlag))
            mode = AddressingMode::Relative;
        else if (allowForIndirectAddressing)
            if (operationMode.modes.testAnyFlags({AddressingModeFlag::IndirectFlag, AddressingModeFlag::IndexedIndirectXFlag, AddressingModeFlag::IndirectIndexedYFlag}))
                mode = AddressingMode::Indirect;

        bool ok;
        bool indirectAddressingMetCloseParen;
        int value = getTokensExpressionValueAsInt(ok, allowForIndirectAddressing, &indirectAddressingMetCloseParen);
        if (!ok)
            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
        if (value < -32768 || value > 65535)
            assemblerWarningMessage(QString("Value out of range for operation: $%1 %2").arg(value, 2, 16, QChar('0')).arg(operationName));
        arg = value;

        if (currentToken.isEmpty())
            recognised = true;
        else
        {
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
            if (recognised)
                getNextToken();
        }
    }
    if (!recognised)
        throw AssemblerError(QString("Unrecognized operand addressing mode for operation: %1 %2").arg(currentToken).arg(operationName));

    bool zpArg = (arg & 0xff00) == 0;
    const Assembly::OperationMode &operationMode(Assembly::getOperationMode(operation));
    switch (mode)
    {
    case AddressingMode::Absolute:
        if (zpArg && operationMode.modes.testFlag(AddressingModeFlag::ZeroPageFlag))
            mode = AddressingMode::ZeroPage;
        break;
    case AddressingMode::AbsoluteX:
        if (zpArg && operationMode.modes.testFlag(AddressingModeFlag::ZeroPageXFlag))
            mode = AddressingMode::ZeroPageX;
        break;
    case AddressingMode::AbsoluteY:
        if (zpArg && operationMode.modes.testFlag(AddressingModeFlag::ZeroPageYFlag))
            mode = AddressingMode::ZeroPageY;
        break;
    case AddressingMode::IndexedIndirectX:
    case AddressingMode::IndirectIndexedY:
        if (!zpArg)
            throw AssemblerError(QString("Operand argument value must be ZeroPage: %1 %2").arg(Assembly::AddressingModeValueToString(mode)).arg(operationName));
        break;
    case AddressingMode::Relative: {
        int relative = arg - _locationCounter - 2;
        if (relative < -128 || relative > 127)
            if (_assembleState == Pass2)
                throw AssemblerError(QString("Relative mode branch address out of range: %1 %2").arg(relative).arg(operationName));
        arg = relative;
        break;
    }
    default: break;
    }

    if (!Assembly::operationSupportsAddressingMode(operation, mode))
        throw AssemblerError(QString("Operation does not support operand addressing mode: %1 %2")
                         .arg(Assembly::OperationValueToString(operation))
                         .arg(Assembly::AddressingModeValueToString(mode)));
}

void Assembler::assembleDirective()
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
                std::memcpy(address, strValue.toLatin1().constData(), len);
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
    }
    else if (directive == ".include")
    {
        getNextToken();
        bool ok;
        QString value = tokenToString(&ok);
        if (!ok)
            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
        getNextToken();
        if (!currentToken.isEmpty())
            throw AssemblerError(QString("Unexpected token at end of statement: %1").arg(currentToken));
        startIncludeFile(value);
        return;
    }
    else if (directive == ".break")
    {
        assemblerBreakpointProvider->addBreakpoint(_locationCounter);
        getNextToken();
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
    }
    else
        throw AssemblerError(QString("Unimplemented directive: %1").arg(directive));
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
    if (_includedFilePaths.contains(includeFilePath))
        return;
    QFile *file = new QFile(includeFilePath);
    if (!file->open(QFile::ReadOnly | QFile::Text))
    {
        QString errorString(file->errorString());
        delete file;
        throw AssemblerError(QString("Could not include file: %1: %2").arg(includeFilePath).arg(errorString));
    }
    _includedFilePaths.append(includeFilePath);

    CodeFileState state(currentFile);

    codeFileStateStack.push(state);

    currentFile.filename = includeFilename;
    currentFile.lineNumber = -1;
    currentFile.file = file;
    currentFile.stream = new QTextStream(file);
    currentFile.lines = QStringList();
    currentToken.clear();
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

QString Assembler::peekNextToken(bool wantOperator /*= false*/)
{
    QString savedToken(currentToken);
    QTextStream &currentLineStream(currentLine.lineStream);
    qint64 savedPos(currentLineStream.pos());

    getNextToken(wantOperator);
    QString nextToken(currentToken);

    currentLineStream.seek(savedPos);
    currentToken = savedToken;
    return nextToken;
}

bool Assembler::getNextToken(bool wantOperator /*= false*/)
{
    currentToken.clear();
    char firstChar, nextChar;

    QTextStream &currentLineStream(currentLine.lineStream);
    auto readChar = [&]() -> char {
        char c = '\0';
        currentLineStream >> c;
        return (currentLineStream.status() == QTextStream::Ok) ? c : '\0';
    };
    do
        firstChar = readChar();
    while (std::isspace(firstChar));

    if (firstChar == '\0')
        return false;
    if (firstChar == ';')
    {
        QString comment = currentLineStream.readLine();
        Q_UNUSED(comment)
        return false;
    }

    currentToken.append(firstChar);
    if (std::ispunct(firstChar) || firstChar == '_')
        if (!(firstChar == '%' || firstChar == '$' || firstChar == '\'' || firstChar == '\"' || firstChar == '_' || firstChar == '.'
              || (!wantOperator && (firstChar == '-' || firstChar == '*' || firstChar == '<' || firstChar == '>'))
              || (wantOperator && (firstChar == '<' || firstChar == '>'))
              ))
            return true;
    nextChar = readChar();
    if (firstChar == '\'')
    {
        if (nextChar != '\0')
        {
            currentToken.append(nextChar);
            nextChar = readChar();
            if (nextChar == '\'')
            {
                currentToken.append(nextChar);
                nextChar = readChar();
            }
        }
    }
    else if (firstChar == '\"')
    {
        while (!(nextChar == '\0' || nextChar == '\"'))
        {
            currentToken.append(nextChar);
            nextChar = readChar();
        }
        if (nextChar == '\"')
        {
            currentToken.append(nextChar);
            nextChar = readChar();
        }
    }
    else if (firstChar == '<' || firstChar == '>')
    {
        if (nextChar == firstChar)
        {
            currentToken.append(nextChar);
            nextChar = readChar();
        }
    }
    else
    {
        while (std::isalnum(nextChar) || nextChar == '_')
        {
            currentToken.append(nextChar);
            nextChar = readChar();
        }
    }
    if (nextChar != '\0')
        currentLineStream.seek(currentLineStream.pos() - 1);
    return true;
}

int Assembler::getTokensExpressionValueAsInt(bool &ok, bool allowForIndirectAddressing /*= false*/, bool *indirectAddressingMetCloseParen /*= nullptr*/)
{
    enum Operator { NotAnOperator = -1, OpenParen, CloseParen, BitOr, BitAnd, ShiftLeft, ShiftRight, Plus, Minus, Multiply, Divide, UnaryLow, UnaryHigh, };
    struct OperatorInfo { Operator _operator; bool isUnary; int precedence; QString string; };
    static const OperatorInfo operatorsInfo[]
    {
        { OpenParen, false, 0, "(", },
        { CloseParen, false, 0, ")", },
        { BitOr, false, 10, "|", },
        { BitAnd, false, 20, "&", },
        { ShiftLeft, false, 30, "<<", },
        { ShiftRight, false, 30, ">>", },
        { Plus, false, 40, "+", },
        { Minus, false, 40, "-", },
        { Multiply, false, 50, "*", },
        { Divide, false, 50, "/", },
        { UnaryLow, true, 100, "<", },
        { UnaryHigh, true, 100, ">", },
    };
    static_assert(NotAnOperator == -1, "Operators::NotAnOperator must have a value of -1");
    static_assert(OpenParen == 0, "Operators::OpenParen must have a value of 0");
    struct LookupOperator
    {
        static Operator find(const QString &_operator)
        {
            for (int i = 0; i < sizeof(operatorsInfo) / sizeof(operatorsInfo[0]); i++)
                if (operatorsInfo[i].string == _operator)
                {
                    Q_ASSERT(operatorsInfo[i]._operator == i);
                    return static_cast<Operator>(i);
                }
            return NotAnOperator;
        }
    };
    QStack<Operator> operatorStack;
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
        Operator _operator = LookupOperator::find(currentToken);

        if (currentToken.isEmpty())
            endOfExpression = true;
        else if (currentToken == "," && allowForIndirectAddressing)
            endOfExpression = true;
        else if (!wantOperator)
        {
            if (_operator != NotAnOperator && operatorsInfo[_operator].isUnary)
            {
            }
            else
            {
                if (_operator == OpenParen)
                    operatorStack.push(OpenParen);
                else
                {
                    value = tokenValueAsInt(&ok);
                    if (!ok)
                        return -1;
                    valueStack.push(value);
                    wantOperator = true;
                }
                _operator = NotAnOperator;
            }
        }
        else
        {
            if (_operator == NotAnOperator)
                endOfExpression = true;
            else if (_operator != CloseParen)
                wantOperator = false;
        }

       if (_operator != NotAnOperator || endOfExpression)
       {
            while (!operatorStack.isEmpty()
                   && operatorsInfo[operatorStack.top()]._operator != OpenParen
                   && (endOfExpression
                      || _operator == CloseParen
                      || operatorsInfo[operatorStack.top()].precedence >= operatorsInfo[_operator].precedence))
            {
                if (operatorStack.size() < 1)
                {
                   ok = false;
                   return -1;
                }
                Operator _operator2 = operatorStack.pop();
                bool isUnary = operatorsInfo[_operator2].isUnary;
                if (valueStack.size() < (isUnary ? 1 : 2))
                {
                    ok = false;
                    return -1;
                }
                int valRight = valueStack.pop();
                int valLeft = isUnary ? -1 : valueStack.pop();
                switch (_operator2)
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
                case UnaryLow:      value = static_cast<uint8_t>(valRight); break;
                case UnaryHigh:      value = static_cast<uint8_t>(valRight >> 8); break;
                default: Q_ASSERT(false); break;
                }
                valueStack.push(value);
            }

            if (_operator != NotAnOperator)
            {
                if (_operator == CloseParen)
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
                    operatorStack.push(_operator);
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
    char ch = currentToken.at(0).toLatin1();
    if (!(ch == '.' || ch == '_' || std::isalpha(ch)))
        return false;
    if (ch == '.')
        if (tokenIsDirective())
            return false;
    for (int i = 1; i < currentToken.size(); i++)
    {
        ch = currentToken.at(i).toLatin1();
        if (!(ch == '_' || std::isalnum(ch)))
            return false;
    }
    return true;
}

bool Assembler::tokenIsInt() const
{
    char firstChar = currentToken.size() > 0 ? currentToken.at(0).toLatin1() : '\0';
    return firstChar == '%' || firstChar == '$' || std::isdigit(firstChar) || firstChar == '-' || firstChar == '\'';
}

int Assembler::tokenToInt(bool *ok) const
{
    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    char firstChar = currentToken.size() > 0 ? currentToken.at(0).toLatin1() : '\0';
    if (firstChar == '%')
        return currentToken.mid(1).toInt(ok, 2);
    else if (firstChar == '$')
        return currentToken.mid(1).toInt(ok, 16);
    else if (std::isdigit(firstChar) || firstChar == '-')
        return currentToken.toInt(ok, 10);
    else if (firstChar == '\'')
    {
        char nextChar = currentToken.size() > 1 ? currentToken.at(1).toLatin1() : '\0';
        char nextChar2 = currentToken.size() > 2 ? currentToken.at(2).toLatin1() : '\0';
        if (nextChar2 == '\'')
        {
            *ok = true;
            return nextChar;
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
