#include <QAbstractItemView>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QToolTip>

#include "codeeditor.h"

//
// CodeEditor Class
//

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit{parent}
{
    completer = new QCompleter(this);
    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    QObject::connect(completer, QOverload<const QString &>::of(&QCompleter::activated),
                     this, &CodeEditor::insertCompletion);

    lineNumberArea = new LineNumberArea(this);
    lineNumberArea->setCursor(Qt::PointingHandCursor);
    codeEditorInfoProvider = nullptr;

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);

    updateLineNumberAreaWidth(0);
}

void CodeEditor::setCodeEditorInfoProvider(const ICodeEditorInfoProvider *provider)
{
    codeEditorInfoProvider = provider;
}

void CodeEditor::moveCursorToEnd()
{
    QTextCursor cursor(textCursor());
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

void CodeEditor::highlightCurrentBlock(QTextBlock &block)
{
    QTextCursor cursor(block), savedTextCursor(textCursor());
    QTextEdit::ExtraSelection selection;
    cursor.movePosition(QTextCursor::StartOfLine);
    selection.cursor = cursor;
    selection.format.setBackground(QColor(255, 220, 180)); // light orange
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    setExtraSelections({ selection });
    setTextCursor(cursor);
    centerCursor();
    // setTextCursor(savedTextCursor);
}

void CodeEditor::unhighlightCurrentBlock()
{
    if (!extraSelections().isEmpty())
        setExtraSelections({});
}

void CodeEditor::handleReturnKey()
{
    QTextCursor tc = textCursor();

    // Get the current block (line)
    QTextBlock block = tc.block();
    QString text = block.text();

    // Extract leading whitespace
    QString indent;
    for (QChar ch : text)
        if (ch == ' ' || ch == '\t')
            indent += ch;
        else
            break;

    tc.beginEditBlock();
    // Insert newline + indent
    tc.insertBlock();
    tc.insertText(indent);
    QChar ch;
    while ((ch = document()->characterAt(tc.position())) == ' ' || ch == '\t')
        tc.deleteChar();
    tc.endEditBlock();

    setTextCursor(tc);
}

void CodeEditor::handleShiftDeleteKey()
{
    QTextCursor tc = textCursor();
    if (tc.hasSelection())
    {
        cut();
        return;
    }
    tc.beginEditBlock();
    // Select the line contents
    tc.select(QTextCursor::LineUnderCursor);
    // Extend selection to include the line break, if there is one
    if (tc.selectionEnd() < tc.document()->characterCount() - 1)
        tc.setPosition(tc.selectionEnd() + 1, QTextCursor::KeepAnchor);
    tc.removeSelectedText();
    tc.endEditBlock();

    setTextCursor(tc);
}

void CodeEditor::handleToggleCommentKey()
{
    QTextCursor tc = textCursor();
    if (!tc.hasSelection())
        tc.select(QTextCursor::LineUnderCursor);

    int selStart = tc.selectionStart();
    int selEnd   = tc.selectionEnd();
    QTextBlock startBlock = tc.document()->findBlock(selStart);

    bool uncomment = true;
    for (QTextBlock block = startBlock; block.isValid(); block = block.next())
    {
        int blockStart = block.position();
        if (blockStart >= selEnd)
            break;

        if (!block.text().startsWith(';'))
            uncomment = false;
    }

    tc.beginEditBlock();
    for (QTextBlock block = startBlock; block.isValid(); block = block.next())
    {
        int blockStart = block.position();
        if (blockStart >= selEnd)
            break;

        QTextCursor lineCursor(tc.document());
        lineCursor.setPosition(blockStart);
        if (uncomment)
        {
            Q_ASSERT(block.text().startsWith(';'));
            lineCursor.deleteChar();
            selEnd--;
        }
        else
        {
            lineCursor.insertText(";");
            selEnd++;
        }
    }
    tc.endEditBlock();
}

bool CodeEditor::handleTabKey()
{
    if (codeEditorInfoProvider == nullptr)
        return false;
    QTextCursor tc = textCursor();
    if (tc.hasSelection())
        return false;

    QTextBlock block = tc.block();
    int endPos = tc.position() - block.position();
    int startPos = endPos;
    char ch;
    while (startPos > 0
           && (std::isalnum(ch = block.text().at(startPos - 1).toLatin1()) || ch == '_' || ch == '.'))
        startPos--;
    QString word = block.text().mid(startPos, endPos - startPos);
    if (word.isEmpty())
        return false;

    QString onlyCompletion = codeEditorInfoProvider->wordCompletion(word, block.blockNumber());
    if (!onlyCompletion.isEmpty())
        tc.insertText(onlyCompletion);
    else
    {
        completer->setModel(codeEditorInfoProvider->wordCompleterModel(block.blockNumber()));
        QAbstractItemView *popup = completer->popup();
        if (word != completer->completionPrefix())
        {
            completer->setCompletionPrefix(word);
        }
        if (!completer->setCurrentRow(0))
            QApplication::beep();
        else if (!completer->setCurrentRow(1))
            tc.insertText(completer->currentCompletion().mid(completer->completionPrefix().length()));
        else
        {
            popup->setCurrentIndex(completer->completionModel()->index(0, 0));
            QRect cr = cursorRect();
            cr.setWidth(popup->sizeHintForColumn(0) + popup->verticalScrollBar()->sizeHint().width());
            completer->complete(cr); // popup it up!
        }
    }

    return true;
}

void CodeEditor::keyPressEvent(QKeyEvent *e) /*override*/
{
    if (completer->popup()->isVisible())
    {
        // The following keys are forwarded by the completer to the widget
        switch (e->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
        default:
            break;
        }
    }
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        handleReturnKey();
        return; // IMPORTANT: do not pass to base class
    }
    else if (e->key() == Qt::Key_Delete && (e->modifiers() & Qt::ShiftModifier))
    {
        handleShiftDeleteKey();
        return;
    }
    else if (e->key() == '/' && (e->modifiers() & Qt::ControlModifier))
    {
        handleToggleCommentKey();
        return;
    }
    else if (e->key() == Qt::Key_Tab)
    {
        if (handleTabKey())
            return;
    }
    QPlainTextEdit::keyPressEvent(e);
}


void CodeEditor::insertCompletion(const QString &completion)
{
    QTextCursor tc = textCursor();
    int extra = completion.length() - completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}


int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }
    int space = 15 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 10;
    return space;
}

/*slot*/ void CodeEditor::updateLineNumberAreaWidth(int newBlockCount)
{
    Q_UNUSED(newBlockCount);
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

/*slot*/ void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

/*slot*/ void CodeEditor::lineNumberAreaBreakpointUpdated(int blockNumber)
{
    if (blockNumber < 0)
    {
        lineNumberArea->update();
        return;
    }
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (!block.isValid())
        return;
    int top = blockBoundingGeometry(block).top() + contentOffset().y();
    int height = blockBoundingRect(block).height();
    QRect rect(0, top - 5, lineNumberAreaWidth(), height + 10);
    lineNumberArea->update(rect);
}

void CodeEditor::resizeEvent(QResizeEvent *e) /*override*/
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(Qt::lightGray).lighter(120));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(10, top, lineNumberArea->width() - 20, fontMetrics().height(),
                             Qt::AlignRight, number);
            if (codeEditorInfoProvider != nullptr)
            {
                ICodeEditorInfoProvider::BreakpointInfo breakpointInfo = codeEditorInfoProvider->findBreakpointInfo(blockNumber);
                if (breakpointInfo.instructionAddress > 0)
                {
                    painter.setBrush(Qt::darkRed);
                    painter.drawEllipse(2, top + 2, 10, 10);
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::lineNumberAreaMousePressEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    emit lineNumberClicked(cursor.blockNumber());
}

void CodeEditor::lineNumberAreaToolTipEvent(QHelpEvent *helpEvent)
{
    QString text;
    if (codeEditorInfoProvider == nullptr)
        return;
    QTextCursor cursor = cursorForPosition(helpEvent->pos());
    int instructionAddress = codeEditorInfoProvider->findInstructionAddress(cursor.blockNumber());
    if (instructionAddress >= 0)
        text = QString("%1").arg(instructionAddress, 4, 16, QChar('0'));
    if (!text.isEmpty())
        QToolTip::showText(helpEvent->globalPos(), text, this);
    else
        QToolTip::hideText();
}
