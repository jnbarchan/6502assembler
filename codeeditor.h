#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QApplication>
#include <QCompleter>
#include <QPlainTextEdit>
#include <QStringListModel>

class LineNumberArea;
class ICodeEditorInfoProvider;

//
// CodeEditor Class
//
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void setCodeEditorInfoProvider(const ICodeEditorInfoProvider *provider);
    void moveCursorToEnd();
    void highlightCurrentBlock(QTextBlock &block);
    void unhighlightCurrentBlock();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void handleReturnKey();
    void handleShiftDeleteKey();
    void handleToggleCommentKey();
    bool handleTabKey();

    QCompleter *completer;
    void insertCompletion(const QString &completion);

signals:
    void lineNumberClicked(int blockNumber);

public:
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void lineNumberAreaMousePressEvent(QMouseEvent *event);
    void lineNumberAreaToolTipEvent(QHelpEvent *helpEvent);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    LineNumberArea *lineNumberArea;
    const ICodeEditorInfoProvider *codeEditorInfoProvider;

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
    {
    }

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
        QApplication::sendEvent(codeEditor->viewport(), event);
        event->accept();
    }
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ToolTip)
        {
            codeEditor->lineNumberAreaToolTipEvent(static_cast<QHelpEvent *>(event));
            return true;
        }
        return QWidget::event(event);
    }

private:
    CodeEditor *codeEditor;
};


//
// ICodeEditorInfoProvider Class
//
class ICodeEditorInfoProvider
{
public:
    struct BreakpointInfo {
        uint16_t instructionAddress;
    };

    virtual ~ICodeEditorInfoProvider() = default;
    virtual BreakpointInfo findBreakpointInfo(int blockNumber) const = 0;
    virtual int findInstructionAddress(int blockNumber) const = 0;

    virtual QString wordCompletion(const QString& word, int lineNumber) const = 0;
    virtual QStringListModel *wordCompleterModel(int lineNumber) const = 0;
};


#endif // CODEEDITOR_H
