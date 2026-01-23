#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QApplication>
#include <QPlainTextEdit>

class LineNumberArea;
class ILineInfoProvider;

//
// CodeEditor Class
//
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void setLineInfoProvider(const ILineInfoProvider *provider);
    void moveCursorToEnd();
    void highlightCurrentBlock(QTextBlock &block);
    void unhighlightCurrentBlock();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void handleReturnKey();
    void handleShiftDeleteKey();

signals:
    void lineNumberClicked(int blockNumber);

public:
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void lineNumberAreaMousePressEvent(QMouseEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    LineNumberArea *lineNumberArea;
    const ILineInfoProvider *lineInfoProvider;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

public slots:
    void lineNumberAreaBreakpointUpdated(int blockNumber);
};


//
// LineNumberArea Class
//
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        codeEditor->lineNumberAreaMousePressEvent(event);
        QWidget::mousePressEvent(event);
    }
    void wheelEvent(QWheelEvent *event) override
    {
        if (auto editor = qobject_cast<CodeEditor *>(parentWidget()))
        {
            QApplication::sendEvent(editor->viewport(), event);
            event->accept();
        }
    }

private:
    CodeEditor *codeEditor;
};


//
// ILineInfoProvider Class
//
class ILineInfoProvider
{
public:
    struct BreakpointInfo {
        uint16_t instructionAddress;
    };

    virtual ~ILineInfoProvider() = default;
    virtual BreakpointInfo findBreakpointInfo(int blockNumber) const = 0;
};


#endif // CODEEDITOR_H
