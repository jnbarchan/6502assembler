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

    connect(this, &QPlainTextEdit::selectionChanged, this, &CodeEditor::addAllSelectedMatchesToExtraSelections);

    lineNumberArea = new LineNumberArea(this);
    lineNumberArea->setCursor(Qt::PointingHandCursor);
    codeEditorInfoProvider = nullptr;

    connect(this, &CodeEditor::foldIndicatorClicked, this, &CodeEditor::toggleFold);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);

    updateLineNumberAreaWidth(0);
}

void CodeEditor::setCodeEditorInfoProvider(const ICodeEditorInfoProvider *provider)
{
    codeEditorInfoProvider = provider;
}

void CodeEditor::highlightCurrentBlock(int blockNumber)
{
    const QColor &color(currentBlockHighlightColor);
    QList<QTextEdit::ExtraSelection> selections(extraSelections());
    selections.removeIf([color](const QTextEdit::ExtraSelection &extraSelection) { return extraSelection.format.background().color() == color; });

    QTextBlock block = document()->findBlockByNumber(blockNumber);
    QTextCursor cursor(block);
    QTextEdit::ExtraSelection selection;
    cursor.movePosition(QTextCursor::StartOfLine);
    selection.cursor = cursor;
    selection.format.setBackground(color);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selections.append(selection);
    setExtraSelections(selections);
    setTextCursor(cursor);
    centerCursor();
}

void CodeEditor::unhighlightCurrentBlock()
{
    const QColor &color(currentBlockHighlightColor);
    QList<QTextEdit::ExtraSelection> selections(extraSelections());
    selections.removeIf([color](const QTextEdit::ExtraSelection &extraSelection) { return extraSelection.format.background().color() == color; });

    setExtraSelections(selections);
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
            completer->setCompletionPrefix(word);
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
            completer->popup()->hide();
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


/*slot*/ void CodeEditor::addAllSelectedMatchesToExtraSelections()
{
    const QColor &color(allMatchesColor);
    QList<QTextEdit::ExtraSelection> selections(extraSelections());
    selections.removeIf([color](const QTextEdit::ExtraSelection &extraSelection) { return extraSelection.format.background().color() == color; });

    setExtraSelections(selections);
    QTextCursor tc = textCursor();
    QString selectedText = tc.selectedText();
    if (selectedText.isEmpty())
        return;
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(color);
    tc.movePosition(QTextCursor::Start);
    while (!(tc = document()->find(selectedText, tc, {QTextDocument::FindCaseSensitively})).isNull())
    {
        selection.cursor = tc;
        selections.append(selection);
    }
    setExtraSelections(selections);
}


/*slot*/ void CodeEditor::setFoldableBlocks(const QList<int> &foldableBlocks)
{
    if (foldableBlocks.isEmpty())  // try to keep whatever we have, e.g. after error
        return;
    QTextBlock firstBlockChanged, lastBlockChanged;
    bool visible = true;
    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next())
    {
        TextBlockFoldData *foldData = nullptr;
        if (foldableBlocks.contains(block.blockNumber()))
        {
            foldData = dynamic_cast<TextBlockFoldData *>(block.userData());
            if (foldData == nullptr)
                foldData = new TextBlockFoldData();
            Q_ASSERT(foldData->isFoldHeader);
        }
        block.setUserData(foldData);

        if (foldData)
            visible = true;
        if (block.isVisible() != visible)
        {
            block.setVisible(visible);
            block.setLineCount(visible ? 1 : 0);
            if (!firstBlockChanged.isValid())
                firstBlockChanged = block;
            lastBlockChanged = block;
        }
        if (foldData)
            visible = !foldData->folded;
    }
    if (firstBlockChanged.isValid())
    {
        document()->markContentsDirty(firstBlockChanged.position(), lastBlockChanged.position());
        postFoldUnfoldAdjust();
    }
}

void CodeEditor::postFoldUnfoldAdjust()
{
    QTextBlock anchorBlock = firstVisibleBlock();
    QPointF anchorOffset = blockBoundingGeometry(anchorBlock).translated(contentOffset()).topLeft();
    int anchorNumber = anchorBlock.blockNumber();
    qreal anchorY = anchorOffset.y();

    QPlainTextDocumentLayout *layout = static_cast<QPlainTextDocumentLayout *>(document()->documentLayout());
    emit layout->documentSizeChanged(layout->documentSize());

    QTextBlock newAnchor = document()->findBlockByNumber(anchorNumber);
    if (!newAnchor.isVisible())
        newAnchor = newAnchor.previous();
    qreal newY = blockBoundingGeometry(newAnchor).translated(contentOffset()).top();
    verticalScrollBar()->setValue(verticalScrollBar()->value() + int(newY - anchorY));

    QTextCursor tc(textCursor());
    while (tc.block().isValid() && !tc.block().isVisible())
        if (!tc.movePosition(QTextCursor::Up))
            break;
    setTextCursor(tc);
}

void CodeEditor::foldUnfold(bool fold, const QTextBlock &fromBlock)
{
    if (!fromBlock.isValid())
        return;
    TextBlockFoldData *foldData = nullptr;
    QTextBlock startBlock = fromBlock;
    while (startBlock.isValid() && ((foldData = dynamic_cast<TextBlockFoldData *>(startBlock.userData())) == nullptr || !foldData->isFoldHeader))
        startBlock = startBlock.previous();
    if (foldData == nullptr || !foldData->isFoldHeader)
        return;

    foldData->folded = fold;
    QTextBlock block = startBlock.next();
    while (block.isValid() && ((foldData = dynamic_cast<TextBlockFoldData *>(block.userData())) == nullptr || !foldData->isFoldHeader))
    {
        block.setVisible(!fold);
        block.setLineCount(!fold ? 1 : 0);
        block = block.next();
    }

    document()->markContentsDirty(startBlock.position(), block.position());
    postFoldUnfoldAdjust();
}

/*slot*/ void CodeEditor::ensureUnfolded(int blockNumber)
{
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (!block.isVisible())
        foldUnfold(false, block);
}

/*slot*/ void CodeEditor::toggleFold(int blockNumber)
{
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    foldUnfold(block.next().isVisible(), block);
}

/*slot*/ void CodeEditor::fold()
{
    foldUnfold(true, textCursor().block());
}

/*slot*/ void CodeEditor::unfold()
{
    foldUnfold(false, textCursor().block());
}

/*slot*/ void CodeEditor::toggleFoldAll()
{
    bool fold = false;
    TextBlockFoldData *foldData;;
    for (QTextBlock block = document()->firstBlock(); block.isValid() && !fold; block = block.next())
        if ((foldData = dynamic_cast<TextBlockFoldData *>(block.userData())) != nullptr && foldData->isFoldHeader && !foldData->folded)
            fold = true;
    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next())
        if ((foldData = dynamic_cast<TextBlockFoldData *>(block.userData())) != nullptr && foldData->isFoldHeader && foldData->folded != fold)
            foldUnfold(fold, block);
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
    int space = 15 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 18;
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
            painter.drawText(10, top, lineNumberArea->width() - 28, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);

            if (codeEditorInfoProvider != nullptr)
            {
                ICodeEditorInfoProvider::BreakpointInfo breakpointInfo = codeEditorInfoProvider->findBreakpointInfo(blockNumber);
                if (breakpointInfo.instructionAddress > 0)
                {
                    painter.setBrush(Qt::darkRed);
                    painter.drawEllipse(2, top + 2, 10, 10);
                }

                // ICodeEditorInfoProvider::BlockFoldingInfo blockFoldingInfo = codeEditorInfoProvider->findBlockFoldingInfo(blockNumber);
                // if (blockFoldingInfo.startBlockNumber == blockNumber)
                // {
                //     QTextBlock nextBlock = block.next();
                //     lineNumberArea->drawFoldIndicator(painter, QRect(lineNumberArea->width() - 14, top, 12, fontMetrics().height()), !nextBlock.isVisible());
                // }
                TextBlockFoldData *foldData = dynamic_cast<TextBlockFoldData *>(block.userData());
                if (foldData && foldData->isFoldHeader)
                    lineNumberArea->drawFoldIndicator(painter, QRect(lineNumberArea->width() - 14, top, 12, fontMetrics().height()), foldData->folded);
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
    if (event->pos().x() >= lineNumberArea->width() - 14)
        emit foldIndicatorClicked(cursor.blockNumber());
    else
        emit lineNumberClicked(cursor.blockNumber());
}

void CodeEditor::lineNumberAreaToolTipEvent(QHelpEvent *helpEvent)
{
    if (codeEditorInfoProvider == nullptr)
        return;
    QTextCursor cursor = cursorForPosition(helpEvent->pos());
    int instructionAddress = codeEditorInfoProvider->findInstructionAddress(cursor.blockNumber());
    QString text;
    if (instructionAddress >= 0)
        text = QString("%1").arg(instructionAddress, 4, 16, QChar('0'));
    if (!text.isEmpty())
        QToolTip::showText(helpEvent->globalPos(), text, this);
    else
        QToolTip::hideText();
}


//
// LineNumberArea Class
//

void LineNumberArea::drawFoldIndicator(QPainter &painter, const QRect &rect, bool folded)
{
    QStyleOption opt;
    opt.rect = rect.adjusted(2, 2, -2, -2);
    opt.state = QStyle::State_Enabled | QStyle::State_Active;

    QStyle::PrimitiveElement pe = folded ? QStyle::PE_IndicatorArrowRight : QStyle::PE_IndicatorArrowDown;

    style()->drawPrimitive(pe, &opt, &painter);
}

