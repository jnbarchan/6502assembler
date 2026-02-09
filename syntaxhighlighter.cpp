#include <QRegularExpression>

#include "assembly.h"
#include "syntaxhighlighter.h"

//
// SyntaxHighlighter Class
//

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{

}

void SyntaxHighlighter::highlightBlock(const QString &text) /*override*/
{
    static const QRegularExpression commentRegex(";.*");
    static const QRegularExpression directiveRegex1("^\\s*[a-z_][a-z_0-9]*\\s*=");
    static const QRegularExpression directiveRegex2 = []{
        QStringList list;
        for (const QString &str : Assembly::directives())
            list.append(QRegularExpression::escape(str));
        QString pattern = QStringLiteral("^\\s*(") + list.join('|') + QStringLiteral(")\\b");
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
    }();
    static const QRegularExpression labelDefinitionRegex("^\\s*(\\.?[a-z_][a-z_0-9]*):", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression labelBranchJumpRegex = []{
        QStringList list;
        QMetaEnum me = Assembly::OperationsMetaEnum();
        for (Assembly::Operation operation : Assembly::branchJumpOperations())
            list.append(QRegularExpression::escape(me.valueToKey(operation)));
        QString pattern = QStringLiteral("\\b(?:") + list.join('|') + QStringLiteral(")\\b");
        pattern += QStringLiteral("\\s+(\\.?[a-z_][a-z_0-9]*)(\\.[a-z_][a-z_0-9]*)?");
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
    }();
    static const QRegularExpression operationRegex = []{
        QStringList list;
        QMetaEnum me = Assembly::OperationsMetaEnum();
        for (int i = 0; i < me.keyCount(); ++i)
            list.append(QRegularExpression::escape(me.valueToKey(i)));
        QString pattern = QStringLiteral("\\b(?:") + list.join('|') + QStringLiteral(")\\b");
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
    }();
    static const QRegularExpression registerRegex("(\\b(A|X|Y)\\b)", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression immediateRegex("#\\S+");

    QTextCharFormat commentFormat;
    commentFormat.setFontItalic(true);
    commentFormat.setForeground(Qt::gray);

    QTextCharFormat directiveFormat;
    directiveFormat.setFontItalic(true);

    QTextCharFormat labelDefinitionFormat;
    labelDefinitionFormat.setFontWeight(QFont::Bold);
    labelDefinitionFormat.setForeground(Qt::magenta);
    QTextCharFormat localLabelDefinitionFormat;
    localLabelDefinitionFormat.setFontWeight(QFont::Bold);
    localLabelDefinitionFormat.setForeground(Qt::darkMagenta);
    QTextCharFormat labelBranchJumpFormat;
    labelBranchJumpFormat.setForeground(labelDefinitionFormat.foreground());
    QTextCharFormat internalLabelBranchJumpFormat;
    QColor color(labelBranchJumpFormat.foreground().color());
    color.setGreen(96);
    internalLabelBranchJumpFormat.setForeground(color);
    QTextCharFormat localLabelBranchJumpFormat;
    localLabelBranchJumpFormat.setForeground(localLabelDefinitionFormat.foreground());

    QTextCharFormat operationFormat;
    operationFormat.setFontWeight(QFont::Bold);
    operationFormat.setForeground(Qt::blue);

    QTextCharFormat registerFormat;
    registerFormat.setForeground(Qt::darkGreen);
    QTextCharFormat immediateFormat;
    immediateFormat.setForeground(Qt::darkRed);

    setCurrentBlockState(0);

    QRegularExpressionMatch match;
    int commentStart = -1, codeEnd = text.length();
    match = commentRegex.match(text);
    if (match.hasMatch())
    {
        commentStart = match.capturedStart();
        setFormat(commentStart, codeEnd - commentStart, commentFormat);
        codeEnd = commentStart;
    }
    QStringView code(text.constData(), codeEnd);

    match = directiveRegex1.match(code);
    if (match.hasMatch())
        setFormat(0, codeEnd, directiveFormat);
    match = directiveRegex2.match(code);
    if (match.hasMatch())
        setFormat(0, codeEnd, directiveFormat);

    match = labelDefinitionRegex.match(code);
    if (match.hasMatch())
        setFormat(match.capturedStart(1), match.capturedLength(1), match.captured(1).at(0) == '.' ? localLabelDefinitionFormat : labelDefinitionFormat);
    match = labelBranchJumpRegex.match(code);
    if (match.hasMatch())
    {
        bool local = match.captured(1).at(0) == '.';
        bool internal = match.captured(1).startsWith("__");
        setFormat(match.capturedStart(1), match.capturedLength(1), local ? localLabelBranchJumpFormat : internal ? internalLabelBranchJumpFormat : labelBranchJumpFormat);
        if (match.hasCaptured(2))
            setFormat(match.capturedStart(2), match.capturedLength(2), localLabelBranchJumpFormat);
    }

    match = operationRegex.match(code);
    if (match.hasMatch())
        setFormat(match.capturedStart(), match.capturedLength(), operationFormat);

    QRegularExpressionMatchIterator it = registerRegex.globalMatch(code);
    while (it.hasNext())
    {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), registerFormat);
    }
    match = immediateRegex.match(code);
    if (match.hasMatch())
        setFormat(match.capturedStart(), match.capturedLength(), immediateFormat);
}
