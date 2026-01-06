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
