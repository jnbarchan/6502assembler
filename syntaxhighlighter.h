#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    SyntaxHighlighter(QTextDocument *parent);

    // QSyntaxHighlighter interface
protected:
    void highlightBlock(const QString &text) override;
};

#endif // SYNTAXHIGHLIGHTER_H
