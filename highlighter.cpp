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
    highlightingRules.append({ QRegularExpression("/"), blackformat });

    blueformat = QTextCharFormat();
    blueformat.setForeground(QColor(0, 0, 255));
    highlightingRules.append({ QRegularExpression(R"(@\d|_|>|#|~)"), blueformat });

    greenformat = QTextCharFormat();
    greenformat.setForeground(QColor(0, 128, 0));
    greenformat.setFontItalic(true);

    highlightingRules.append({ QRegularExpression(R"(\*.*)"), greenformat });

    categoryformat = QTextCharFormat();
    categoryformat.setBackground(QColor(245, 245, 220));
    highlightingRules.append({ QRegularExpression(categoryregexp), categoryformat });
    highlightingRules.append({ QRegularExpression(R"(\[.*?\])"), categoryformat });
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
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}