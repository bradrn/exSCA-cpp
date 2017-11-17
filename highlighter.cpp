#include <qstring.h>
#include <QColor>
#include <QFont>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QTextCharFormat>
#include <QTextDocument>
#include "highlighter.h"

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    MakeHighlightingRules(QString(""));
}

void Highlighter::MakeHighlightingRules(QString categoryregexp)
{
    highlightingRules = QVector<HighlightingRule>();

    blackformat = QTextCharFormat();
    blackformat.setFontWeight(QFont::Bold);

    blueformat = QTextCharFormat();
    blueformat.setForeground(QColor(0, 0, 255));

    greenformat = QTextCharFormat();
    greenformat.setForeground(QColor(0, 128, 0));
    greenformat.setFontItalic(true);

    categoryformat = QTextCharFormat();
    categoryformat.setBackground(QColor(245, 245, 220));

    // make sure to append them in the right order
    highlightingRules.append({ QRegularExpression(R"((@\d|_|>|#|~|`))"), blueformat });
    highlightingRules.append({ QRegularExpression("(/)"), blackformat });
    highlightingRules.append({ QRegularExpression("(" + categoryregexp + ")"), categoryformat });
    highlightingRules.append({ QRegularExpression(R"((\[.*?\]))"), categoryformat });
    highlightingRules.append({ QRegularExpression(R"(^(?:[^ ]+ (?:[^ ]+ )*)?(?:[^_][^ ]*)?(\*.*))"), greenformat });
}

void Highlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule rule : highlightingRules)
    {
        QRegularExpression expression(rule.pattern);
        QRegularExpressionMatchIterator matches = expression.globalMatch(text);
        while (matches.hasNext())
        {
            QRegularExpressionMatch match = matches.next();
            setFormat(match.capturedStart(1), match.capturedLength(1), rule.format);
        }
    }
}