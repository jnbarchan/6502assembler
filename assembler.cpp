#include <QDebug>
#include <QDir>
#include <QFileInfo>

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

const Assembler::CodeLabels &Assembler::codeLabels() const
{
    return _codeLabels;
}

Assembler::ExpressionValue Assembler::codeLabelValue(const QString &key) const
{
    return _codeLabels.values.value(key, ExpressionValue(true, true, -1));
}

void Assembler::setCodeLabelValue(const QString &key, ExpressionValue value)
{
    Q_ASSERT(value.ok);
    _codeLabels.values[key] = value;
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

void Assembler::assignLabelValue(const QString &scopedLabel, bool isLabel, ExpressionValue value)
{
    if (scopedLabel.isEmpty())
        assemblerWarningMessage(QString("Defining label with no name"));
    if (_codeLabels.values.contains(scopedLabel))
    {
        ExpressionValue labelValue(_codeLabels.values.value(scopedLabel));
        if (!labelValue.isUndefined && !value.isUndefined && labelValue.intValue != value.intValue)
            assemblerWarningMessage(QString("Label redefinition: %1").arg(scopedLabel));
    }
    setCodeLabelValue(scopedLabel, value);
    if (isLabel && !scopedLabel.contains('.'))
    {
        _currentCodeLabelScope = scopedLabel;
        _codeLabels.scopes[currentFile.filename].append(ScopeLabel(scopedLabel, currentFile.lineNumber));
    }
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
    _currentCodeLabelScope.clear();
    currentToken.clear();
    _codeLabels.scopes.clear();

    if (assemblePass2)
        return;

    assemblerBreakpointProvider->clearBreakpoints();

    struct InternalJSR { const char *label; int intValue; };
    const InternalJSR internals[]
    {
        {"__terminate", InternalJSRs::__JSR_terminate},
        {"__brk_handler", InternalJSRs::__JSR_brk_handler},
        {"__brk_default_handler", InternalJSRs::__JSR_brk_default_handler},
        {"__outch", InternalJSRs::__JSR_outch},
        {"__get_time", InternalJSRs::__JSR_get_time},
        {"__get_time_ms", InternalJSRs::__JSR_get_time_ms},
        {"__get_elapsed_time", InternalJSRs::__JSR_get_elapsed_time},
        {"__clear_elapsed_time", InternalJSRs::__JSR_clear_elapsed_time},
        {"__process_events", InternalJSRs::__JSR_process_events},
        {"__inch", InternalJSRs::__JSR_inch},
        {"__inkey", InternalJSRs::__JSR_inkey},
        {"__wait", InternalJSRs::__JSR_wait},
        {"__open_file", InternalJSRs::__JSR_open_file},
        {"__close_file", InternalJSRs::__JSR_close_file},
        {"__rewind_file", InternalJSRs::__JSR_rewind_file},
        {"__read_file", InternalJSRs::__JSR_read_file},
        {"__outstr_fast", InternalJSRs::__JSR_outstr_fast},
        {"__outstr_inline", InternalJSRs::__JSR_outstr_inline},
        {"__get_elapsed_cycles", InternalJSRs::__JSR_get_elapsed_cycles},
        {"__clear_elapsed_cycles", InternalJSRs::__JSR_clear_elapsed_cycles},

        {"__BRKV", InternalVECs::__VEC_BRKV},

        {NULL, -1}
    };
    _codeLabels.values.clear();
    for (const InternalJSR *internal = internals; internal->label != NULL; internal++)
        _codeLabels.values[internal->label] = internal->intValue;

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
    uint16_t intValue;
    bool hasOperation, eof;
    assembleNextStatement(operation, mode, intValue, hasOperation, eof);
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
            *instruction = Instruction(instructionInfo->opcodeByte, intValue);
            address[0] = instructionInfo->opcodeByte;
            if (bytes > 1)
            {
                address[1] = static_cast<uint8_t>(intValue);
                if (bytes > 2)
                    address[2] = static_cast<uint8_t>(intValue >> 8);
            }
            setLocationCounter(_locationCounter + bytes);
        }
        setCurrentCodeLineNumber(currentFile.lineNumber + 1);
        assembleNextStatement(operation, mode, intValue, hasOperation, eof);
    }
    for (int i = 1; i < _instructionsCodeFileLineNumbers.size(); i++)
        Q_ASSERT(_instructionsCodeFileLineNumbers.at(i)._locationCounter > _instructionsCodeFileLineNumbers.at(i - 1)._locationCounter);
}

void Assembler::assembleNextStatement(Operation &operation, AddressingMode &mode, uint16_t &intValue, bool &hasOperation, bool &eof)
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
    assembleNextStatement(operation, mode, intValue, hasOperation);
    if (!currentToken.isEmpty())
        throw AssemblerError(QString("Unexpected token at end of statement: %1").arg(currentToken));
}

void Assembler::assembleNextStatement(Operation &operation, AddressingMode &mode, uint16_t &intValue, bool &hasOperation)
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
        if (tokenIsLabel(true))
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
                ExpressionValue value = getTokensExpressionValueAsInt();
                if (!value.ok)
                    throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                assignLabelValue(scopedLabelName(label), false, value);
                return;
            }
            else if (isCodeLabel)
            {
                assignLabelValue(scopedLabelName(label), true, _locationCounter);
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
        intValue = 0;
        recognised = true;
    }
    else if (currentToken == '#')
    {
        mode = AddressingMode::Immediate;
        getNextToken();
        ExpressionValue value = getTokensExpressionValueAsInt();
        if (!value.ok)
            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
        if (value.isUndefined)
            intValue = 0;
        else
        {
            if (value.intValue < -128 || value.intValue > 255)
                assemblerWarningMessage(QString("Value out of range for operation: $%1 %2").arg(value.intValue, 2, 16, QChar('0')).arg(operationName));
            intValue = value.intValue;
        }
        recognised = true;
    }
    else if (currentToken.toUpper() == "A")
    {
        mode = AddressingMode::Accumulator;
        intValue = 0;
        getNextToken();
        recognised = true;
    }
    else if (tokenStartsExpression())
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

        bool indirectAddressingMetCloseParen;
        ExpressionValue value = getTokensExpressionValueAsInt(allowForIndirectAddressing, &indirectAddressingMetCloseParen);
        if (!value.ok)
            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
        if (value.isUndefined)
            intValue = 0xffff;
        else
        {
            if (value.intValue < -32768 || value.intValue > 65535)
                assemblerWarningMessage(QString("Value out of range for operation: $%1 %2").arg(value.intValue, 2, 16, QChar('0')).arg(operationName));
            intValue = value.intValue;
        }

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

    bool zpArg = (intValue & 0xff00) == 0;
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
        int relative = intValue - _locationCounter - 2;
        if (relative < -128 || relative > 127)
            if (_assembleState == Pass2)
                throw AssemblerError(QString("Relative mode branch address out of range: %1 %2").arg(relative).arg(operationName));
        intValue = relative;
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

    if (directive == ".byte" || directive == ".word")
    {
        int size = directive == ".word" ? 2 : 1;
        do
        {
            uint8_t *memoryAddress = reinterpret_cast<uint8_t *>(_instructions) + _locationCounter;
            getNextToken();
            bool ok;
            if (size == 1 && tokenIsString())
            {
                QString strValue = tokenToString(&ok);
                if (!ok)
                    throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                int len = strValue.length();
                std::memcpy(memoryAddress, strValue.toLatin1().constData(), len);
                setLocationCounter(_locationCounter + len);
                getNextToken();
            }
            else
            {
                ExpressionValue value = getTokensExpressionValueAsInt();
                if (!value.ok)
                    throw AssemblerError(QString("Bad value: %1").arg(currentToken));
                if (!value.isUndefined)
                {
                    *memoryAddress++ = static_cast<uint8_t>(value.intValue);
                    if (size > 1)
                        *memoryAddress++ = static_cast<uint8_t>(value.intValue >> 8);
                }
                setLocationCounter(_locationCounter + size);
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
        ExpressionValue value = getTokensExpressionValueAsInt();
        if (!value.ok)
            throw AssemblerError(QString("Bad value: %1").arg(currentToken));
        if (!value.isUndefined)
        {
            int intValue = value.intValue;
            if (intValue < 0 || intValue > 0xffff)
                assemblerWarningMessage(QString("Value out of range for directive: $%1 %2").arg(intValue, 2, 16, QChar('0')).arg(directive));
            setLocationCounter(intValue);
        }
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
    _currentCodeLabelScope.clear();
}

void Assembler::endIncludeFile()
{
    if (codeFileStateStack.isEmpty())
        return;

    closeIncludeFile();

    currentFile = codeFileStateStack.pop();
    _currentCodeLabelScope.clear();
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
        while (std::isalnum(nextChar) || nextChar == '_' || nextChar == '.')
        {
            currentToken.append(nextChar);
            nextChar = readChar();
        }
    }
    if (nextChar != '\0')
        currentLineStream.seek(currentLineStream.pos() - 1);
    return true;
}

Assembler::ExpressionValue Assembler::getTokensExpressionValueAsInt(bool allowForIndirectAddressing /*= false*/, bool *indirectAddressingMetCloseParen /*= nullptr*/)
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
    QStack<ExpressionValue> valueStack;
    ExpressionValue value;

    if (indirectAddressingMetCloseParen != nullptr)
        *indirectAddressingMetCloseParen = false;
    QString firstToken(currentToken);
    if (firstToken != "(")
        allowForIndirectAddressing = false;
    value.ok = false;
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
                    value = tokenValueAsInt();
                    if (!value.ok)
                        return value;
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
                   value.ok = false;
                   return value;
                }
                Operator _operator2 = operatorStack.pop();
                bool isUnary = operatorsInfo[_operator2].isUnary;
                if (valueStack.size() < (isUnary ? 1 : 2))
                {
                    value.ok = false;
                    return value;
                }
                ExpressionValue valueRight = valueStack.pop();
                Q_ASSERT(valueRight.ok);
                ExpressionValue valueLeft = isUnary ? ExpressionValue(-1) : valueStack.pop();
                Q_ASSERT(valueLeft.ok);
                if (valueRight.isUndefined || valueLeft.isUndefined)
                    value = ExpressionValue(true, true, -1);
                else
                {
                    int valRight = valueRight.intValue;
                    int valLeft = valueLeft.intValue;
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
                            value.ok = false;
                            return value;
                        }
                        value = valLeft / valRight;
                        break;
                    case UnaryLow:       value = static_cast<uint8_t>(valRight); break;
                    case UnaryHigh:      value = static_cast<uint8_t>(valRight >> 8); break;
                    default: Q_ASSERT(false); break;
                    }
                }
                Q_ASSERT(value.ok);
                valueStack.push(value);
            }

            if (_operator != NotAnOperator)
            {
                if (_operator == CloseParen)
                {
                    if (operatorStack.isEmpty() || operatorsInfo[operatorStack.top()]._operator != OpenParen)
                    {
                        value.ok = false;
                        return value;
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

    value.ok = false;
    if (valueStack.size() != 1)
        return value;

    if (allowForIndirectAddressing)
        if (currentToken == "," && operatorStack.size() == 1 && operatorsInfo[operatorStack.top()]._operator == OpenParen)
            operatorStack.pop();
    if (!operatorStack.isEmpty())
        return value;

    value.ok = true;
    return value;
}

bool Assembler::tokenIsDirective() const
{
    if (!currentToken.startsWith('.'))
        return false;
    return Assembly::directives().contains(currentToken);
}

bool Assembler::tokenStartsExpression() const
{
    return tokenIsInt() || tokenIsLabel() || currentToken == "*"  || currentToken == "<"  || currentToken == ">" || currentToken == "(";
}

bool Assembler::tokenIsLabel(bool isDefinition /*= false*/) const
{
    if (currentToken.isEmpty())
        return false;
    char ch = currentToken.at(0).toLatin1();
    if (!(ch == '.' || ch == '_' || std::isalpha(ch)))
        return false;
    if (tokenIsDirective())
        return false;
    for (int i = 1; i < currentToken.size(); i++)
    {
        ch = currentToken.at(i).toLatin1();
        if (!(ch == '_' || std::isalnum(ch) || (ch == '.' && !isDefinition)))
            return false;
    }
    return true;
}

bool Assembler::tokenIsInt() const
{
    if (currentToken.isEmpty())
        return false;
    char firstChar = currentToken.at(0).toLatin1();
    return firstChar == '%' || firstChar == '$' || std::isdigit(firstChar) || firstChar == '-' || firstChar == '\'';
}

int Assembler::tokenToInt(bool *ok) const
{
    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    *ok = false;
    if (currentToken.isEmpty())
        return -1;
    char firstChar = currentToken.at(0).toLatin1();
    if (firstChar == '%')
        return currentToken.mid(1).toUInt(ok, 2);
    else if (firstChar == '$')
        return currentToken.mid(1).toUInt(ok, 16);
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
    return -1;
}

Assembler::ExpressionValue Assembler::tokenValueAsInt() const
{
    ExpressionValue value;
    value.isUndefined = false;
    value.intValue = tokenToInt(&value.ok);
    if (value.ok)
        return value;
    if (tokenIsLabel())
    {
        QString label = scopedLabelName(currentToken);
        if (_codeLabels.values.contains(label))
            return _codeLabels.values.value(label);
        else if (_assembleState == Pass1)
        {
            value.ok = true;
            value.isUndefined = true;
            return value;
        }
        assemblerWarningMessage(QString("Label not defined: %1").arg(currentToken));
    }
    else if (currentToken == "*")
    {
        value = _locationCounter;
        return value;
    }
    value.ok = false;
    return value;
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
