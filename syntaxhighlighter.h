#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <functional>

#include <QSyntaxHighlighter>

//
// SyntaxHighlighter Class
//
class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    SyntaxHighlighter(QTextDocument *parent);

    std::function<QStringList()> getMacroNames = nullptr;

    // QSyntaxHighlighter interface
protected:
    void highlightBlock(const QString &text) override;
};

#endif // SYNTAXHIGHLIGHTER_H
