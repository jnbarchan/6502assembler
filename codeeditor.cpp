#include <QTextBlock>

#include "codeeditor.h"

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit{parent}
{
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
    QTextCursor c = textCursor();

    // Get the current block (line)
    QTextBlock block = c.block();
    QString text = block.text();

    // Extract leading whitespace
    QString indent;
    for (QChar ch : text)
        if (ch == ' ' || ch == '\t')
            indent += ch;
        else
            break;

    c.beginEditBlock();
    // Insert newline + indent
    c.insertBlock();
    c.insertText(indent);
    c.endEditBlock();

    setTextCursor(c);
}

void CodeEditor::handleShiftDeleteKey()
{
    QTextCursor c = textCursor();
    if (c.hasSelection())
    {
        cut();
        return;
    }
    c.beginEditBlock();
    // Select the line contents
    c.select(QTextCursor::LineUnderCursor);
    // Extend selection to include the line break, if there is one
    if (c.selectionEnd() < c.document()->characterCount() - 1)
        c.setPosition(c.selectionEnd() + 1, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    c.endEditBlock();

    setTextCursor(c);
}

void CodeEditor::keyPressEvent(QKeyEvent *e) /*override*/
{
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
    QPlainTextEdit::keyPressEvent(e);
}
