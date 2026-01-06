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

protected:
    void keyPressEvent(QKeyEvent *e) override;

signals:

private:
    void handleReturnKey();
    void handleShiftDeleteKey();
};

#endif // CODEEDITOR_H
