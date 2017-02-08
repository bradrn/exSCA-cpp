#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class QTextCharFormat;
template <class T> class QVector;

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);
    void MakeHighlightingRules(QString categoryregexp);

protected:
    void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat blackformat;
    QTextCharFormat blueformat;
    QTextCharFormat greenformat;
    QTextCharFormat categoryformat;
};

#endif
