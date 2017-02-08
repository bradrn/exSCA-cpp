#include <QWidget>
#include <QString>
#include <QStringList>
#include <QList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <utility>

#include "affixerdialog.h"

AffixerDialog::AffixerDialog(QWidget *parent) : QDialog(parent)
{
    m_toplayout = new QGridLayout;
    m_bottomlayout = new QHBoxLayout;
    m_layout = new QVBoxLayout;
    this->setLayout(m_layout);

    m_prefixeslabel = new QLabel("Prefixes:");
    m_toplayout->addWidget(m_prefixeslabel, 0, 0);

    m_rootslabel = new QLabel("Roots:");
    m_toplayout->addWidget(m_rootslabel, 0, 1);

    m_infixeslabel = new QLabel("Infixes:");
    m_toplayout->addWidget(m_infixeslabel, 0, 2);

    m_suffixeslabel = new QLabel("Suffixes:");
    m_toplayout->addWidget(m_suffixeslabel, 0, 3);

    m_previewlabel = new QLabel("Preview:");
    m_toplayout->addWidget(m_previewlabel, 0, 4);

    m_prefixes = new QPlainTextEdit;
    m_prefixes->setPlainText("*");
    m_toplayout->addWidget(m_prefixes, 1, 0);

    m_roots = new QPlainTextEdit;
    m_toplayout->addWidget(m_roots, 1, 1);

    m_infixes = new QPlainTextEdit;
    m_infixes->setPlainText("1");
    m_toplayout->addWidget(m_infixes, 1, 2);

    m_suffixes = new QPlainTextEdit;
    m_suffixes->setPlainText("*");
    m_toplayout->addWidget(m_suffixes, 1, 3);

    m_preview = new QPlainTextEdit;
    m_preview->setReadOnly(true);
    m_toplayout->addWidget(m_preview, 1, 4);

    m_addToStart = new QPushButton("Add to start");
    m_bottomlayout->addWidget(m_addToStart);

    m_addToEnd = new QPushButton("Add to end");
    m_bottomlayout->addWidget(m_addToEnd);

    m_addAtCursor = new QPushButton("Add at cursor position");
    m_bottomlayout->addWidget(m_addAtCursor);

    m_overwritetext = new QPushButton("Replace text");
    m_bottomlayout->addWidget(m_overwritetext);

    m_layout->addLayout(m_toplayout);
    m_layout->addLayout(m_bottomlayout);

    connect(m_prefixes, &QPlainTextEdit::textChanged, this, &AffixerDialog::textEntered);
    connect(m_roots, &QPlainTextEdit::textChanged, this, &AffixerDialog::textEntered);
    connect(m_infixes, &QPlainTextEdit::textChanged, this, &AffixerDialog::textEntered);
    connect(m_suffixes, &QPlainTextEdit::textChanged, this, &AffixerDialog::textEntered);
    connect(m_addToEnd, &QPushButton::clicked, this, &AffixerDialog::emitAddTextToEnd);
    connect(m_addToStart, &QPushButton::clicked, this, &AffixerDialog::emitAddTextToStart);
    connect(m_addAtCursor, &QPushButton::clicked, this, &AffixerDialog::emitAddTextAtCursor);
    connect(m_overwritetext, &QPushButton::clicked, this, &AffixerDialog::emitOverwriteText);
}

void AffixerDialog::textEntered()
{
    m_preview->setPlainText(this->GetWords().join('\n'));
}

void AffixerDialog::emitAddTextToEnd()
{
    emit addText(this->GetWords(), PlaceToAdd::AddToEnd);
}

void AffixerDialog::emitAddTextToStart()
{
    emit addText(this->GetWords(), PlaceToAdd::AddToStart);
}

void AffixerDialog::emitAddTextAtCursor()
{
    emit addText(this->GetWords(), PlaceToAdd::AddAtCursor);
}

void AffixerDialog::emitOverwriteText()
{
    emit addText(this->GetWords(), PlaceToAdd::Overwrite);
}

QStringList AffixerDialog::GetWords()
{
    QStringList words;
    QStringList roots = m_roots->toPlainText().split('\n', QString::SkipEmptyParts);

    QList<QStringList> prefixSlots = GetSlotAffixes(m_prefixes->toPlainText().split('\n', QString::SkipEmptyParts));
    QList<QStringList> infixSlots  = GetSlotAffixes(m_infixes ->toPlainText().split('\n', QString::SkipEmptyParts));
    QList<QStringList> suffixSlots = GetSlotAffixes(m_suffixes->toPlainText().split('\n', QString::SkipEmptyParts));

    QList<QStringList> prefixcombs = CartesianProduct(prefixSlots);
    QList<QStringList> infixcombs  = CartesianProduct(infixSlots);
    QList<QStringList> suffixcombs = CartesianProduct(suffixSlots);

    QList<int> infixnums;
    for (QStringList infixSlot : infixSlots)
    {
        for (QString infix : infixSlot)
        {
            QRegularExpressionMatch match = QRegularExpression(R"re(^(\d+)(.*)$)re").match(infix);
            int num = match.captured(1).toInt();
            if (!infixnums.contains(num)) infixnums.append(num);
        }
    }
    for (QString root : roots)
    {
        for (QStringList prefixcomb : prefixcombs)
        {
            for (QStringList infixcomb : infixcombs)
            {
                QList<std::pair<int, QString>> infixes;
                for (QString infix : infixcomb)
                {
                    QRegularExpressionMatch match = QRegularExpression(R"re(^(\d+)(.*)$)re").match(infix);
                    if (match.hasMatch())
                    {
                        int num = match.captured(1).toInt();
                        infixes.append(std::make_pair(num, match.captured(2)));
                    }
                }

                for (QStringList suffixcomb : suffixcombs)
                {
                    QString word(root);
                    for (std::pair<int, QString> infix : infixes)
                    {
                        word = word.replace(QString::number(infix.first), infix.second);
                    }

                    for (int infixnum : infixnums)
                    {
                        word = word.replace(QString::number(infixnum), "");
                    }

                    word = prefixcomb.join("").append(word);
                    word = word.append(suffixcomb.join(""));
                    words.append(word);
                }
            }
        }
    }

    return words;
}

QList<QStringList> AffixerDialog::GetSlotAffixes(QStringList affixGroups)
{
    QList<QStringList> result;
    for (QString slot : affixGroups)
    {
        QStringList affixes;
        QStringList splitSlots = slot.split(' ', QString::SkipEmptyParts);
        for (QString affix : splitSlots)
        {
            affixes.append((affix == "*") ? "" : affix);
        }
        result.append(affixes);
    }
    return result;
}

QList<QStringList> AffixerDialog::CartesianProduct(QList<QStringList> sequences)
{
    if (sequences.count() == 0) return QList<QStringList>();

    QList<QStringList> result;
    for (QString s : sequences.at(0))
    {
        result.append({ QStringList(s) });
    }

    for (int i = 1; i < sequences.count(); i++)
    {
        QList<QStringList> temp;
        for (QStringList sl : result)
        {
            for (QString s : sequences.at(i))
            {
                QStringList tempStrList = QStringList(sl);
                tempStrList.append(s);
                temp.append(tempStrList);
            }
        }
        result = QList<QStringList>(temp);
    }

    return result;
}