#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void moveCursorToEnd();
    void highlightCurrentBlock(QTextBlock &block);
    void unhighlightCurrentBlock();

signals:
};

#endif // CODEEDITOR_H
