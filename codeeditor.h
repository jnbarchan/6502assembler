#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QApplication>
#include <QCompleter>
#include <QPlainTextEdit>
#include <QStringListModel>
#include <QTextBlock>

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
    void highlightCurrentBlock(int blockNumber);
    void unhighlightCurrentBlock();

    void setFoldableBlocks(const QList<int> &foldableBlocks);

    QList<int> breakpointBlocks() const;
    void setBreakpointBlocks(const QList<int> &breakpointBlocks);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void handleReturnKey();
    void handleShiftDeleteKey();
    void handleToggleCommentKey();
    bool handleTabKey();

    QCompleter *completer;
    void insertCompletion(const QString &completion);

private slots:
    void addAllSelectedMatchesToExtraSelections();

signals:
    void lineNumberClicked(int blockNumber);
    void foldIndicatorClicked(int blockNumber);

public:
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth() const;
    void lineNumberAreaMousePressEvent(QMouseEvent *event);
    void lineNumberAreaToolTipEvent(QHelpEvent *helpEvent);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    struct TextBlockFoldData
    {
        bool isFoldHeader = false;
        bool folded = false;
    };
    struct TextBlockBreakpointData
    {
        bool hasBreakpoint = false;
    };
    struct CodeEditorTextBlockUserData : public QTextBlockUserData
    {
        TextBlockFoldData foldData;
        TextBlockBreakpointData breakpointData;
    };
    CodeEditorTextBlockUserData *makeTextBlockUserData(QTextBlock block);
    TextBlockFoldData *blockFoldData(const QTextBlock &block) const;
    TextBlockBreakpointData *blockBreakpointData(const QTextBlock &block) const;

    void postFoldUnfoldAdjust();
    void foldUnfold(bool fold, const QTextBlock &fromBlock);

    const QColor currentBlockHighlightColor = QColor(255, 220, 180);  // light orange
    const QColor allMatchesColor = QColor("#ccdfec");  // light blue

    LineNumberArea *lineNumberArea;
    const ICodeEditorInfoProvider *codeEditorInfoProvider;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

public slots:
    void ensureUnfolded(int blockNumber);
    void toggleFold(int blockNumber);
    void fold();
    void unfold();
    void toggleFoldAll();

    void lineNumberAreaBreakpointUpdated(int blockNumber, bool hasBreakpoint);
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

    void drawFoldIndicator(QPainter &painter, const QRect &rect, bool folded);
    void drawLineNumber(QPainter &painter, const QRect &rect, int lineNumber);
    void drawBreakpointIndicator(QPainter &painter, const QRect &rect);

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
    virtual ~ICodeEditorInfoProvider() = default;

    struct BreakpointInfo
    {
        uint16_t instructionAddress;
    };

    virtual BreakpointInfo findBreakpointInfo(int blockNumber) const = 0;
    virtual uint16_t findInstructionAddress(int blockNumber) const = 0;

    virtual QString wordCompletion(const QString& word, int lineNumber) const = 0;
    virtual QStringListModel *wordCompleterModel(int lineNumber) const = 0;
};


#endif // CODEEDITOR_H
