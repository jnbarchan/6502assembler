#include <QRegularExpression>

#include "assembly.h"
#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{

}

void SyntaxHighlighter::highlightBlock(const QString &text) /*override*/
{
    static const QRegularExpression commentRegex(";.*");
    static const QRegularExpression directiveRegex("^\\s*[a-z_][a-z_0-9]*\\s*=");
    static const QRegularExpression labelDefinitionRegex("^\\s*(\\.?[a-z_][a-z_0-9]*):", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression labelBranchJumpRegex = []{
        QStringList list;
        QMetaEnum me = Assembly::OpcodesMetaEnum();
        for (Assembly::Opcodes opcode : Assembly::branchJumpOpcodes())
            list.append(QRegularExpression::escape(me.valueToKey(opcode)));
        QString pattern = QStringLiteral("\\b(?:") + list.join('|') + QStringLiteral(")\\b");
        pattern += QStringLiteral("\\s+(\\.?[a-z_][a-z_0-9]*)");
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
    }();
    static const QRegularExpression opcodeRegex = []{
        QStringList list;
        QMetaEnum me = Assembly::OpcodesMetaEnum();
        for (int i = 0; i < me.keyCount(); ++i)
            list.append(QRegularExpression::escape(me.valueToKey(i)));
        QString pattern = QStringLiteral("\\b(?:") + list.join('|') + QStringLiteral(")\\b");
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
    }();
    static const QRegularExpression registerRegex("(\\b(A|X|Y|SP|PC)\\b)", QRegularExpression::CaseInsensitiveOption);
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
    QTextCharFormat localLabelBranchJumpFormat;
    localLabelBranchJumpFormat.setForeground(localLabelDefinitionFormat.foreground());

    QTextCharFormat opcodeFormat;
    opcodeFormat.setFontWeight(QFont::Bold);
    opcodeFormat.setForeground(Qt::blue);

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

    match = directiveRegex.match(code);
    if (match.hasMatch())
        setFormat(0, codeEnd, directiveFormat);

    match = labelDefinitionRegex.match(code);
    if (match.hasMatch())
        setFormat(match.capturedStart(1), match.capturedLength(1), match.captured(1).at(0) == '.' ? localLabelDefinitionFormat : labelDefinitionFormat);
    match = labelBranchJumpRegex.match(code);
    if (match.hasMatch())
        setFormat(match.capturedStart(1), match.capturedLength(1), match.captured(1).at(0) == '.' ? localLabelBranchJumpFormat : labelBranchJumpFormat);

    match = opcodeRegex.match(code);
    if (match.hasMatch())
        setFormat(match.capturedStart(), match.capturedLength(), opcodeFormat);

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
