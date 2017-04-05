#include <QtCore>
#include <QtWidgets>
#include <random>

#include "window.h"
#include "soundchanges.h"
#include "highlighter.h"
#include "affixerdialog.h"

Window::Window()
{
    QWidget *mainwidget = new QWidget;
    setCentralWidget(mainwidget);

    m_layout = new QHBoxLayout;
    m_leftlayout = new QVBoxLayout;
    m_midlayout = new QVBoxLayout;
    m_syllableseperatorlayout = new QHBoxLayout;
    mainwidget->setLayout(m_layout);

    QFont font("Courier", 10);
    font.setFixedPitch(true);

    m_categories = new QPlainTextEdit;
    m_categories->setFont(font);
    m_leftlayout->addWidget(m_categories, 2);

    m_rewrites = new QPlainTextEdit;
    m_leftlayout->addWidget(m_rewrites, 1);

    m_layout->addLayout(m_leftlayout);

    m_rules = new QPlainTextEdit;
    m_rules->setFont(font);
    m_highlighter = new Highlighter(m_rules->document());
    m_layout->addWidget(m_rules);

    m_words = new QPlainTextEdit;
    m_layout->addWidget(m_words);

    m_apply = new QPushButton("Apply");
    m_midlayout->addWidget(m_apply);

    m_showChangedWords = new QCheckBox("Show changed words");
    m_showChangedWords->setChecked(true);
    m_midlayout->addWidget(m_showChangedWords);

    m_reportChanges = new QCheckBox("Report which rules apply");
    m_midlayout->addWidget(m_reportChanges);

    m_doBackwards = new QCheckBox("Rewrite on output");
    m_midlayout->addWidget(m_doBackwards);

    const QChar arrow(0x2192);
    m_formatgroup = new QGroupBox("Output format");
    m_plainformat = new QRadioButton("output > gloss");
    m_arrowformat = new QRadioButton(QString("input %1 output").arg(arrow));
    m_squareinputformat = new QRadioButton("output [input]");
    m_squareglossformat = new QRadioButton("output [gloss]");
    m_arrowglossformat = new QRadioButton(QString("input %1 output [gloss]").arg(arrow));
    QVBoxLayout *fbox = new QVBoxLayout;
    fbox->addWidget(m_plainformat);
    fbox->addWidget(m_arrowformat);
    fbox->addWidget(m_squareinputformat);
    fbox->addWidget(m_squareglossformat);
    fbox->addWidget(m_arrowglossformat);
    m_plainformat->setChecked(true);
    m_formatgroup->setLayout(fbox);
    m_midlayout->addWidget(m_formatgroup);
    
    m_reversegroup = new QGroupBox("Reversing changes");
    m_reversechanges = new QCheckBox("Reverse changes");
    m_filters = new QPlainTextEdit;
    m_filterslabel = new QLabel("Filters:");
    QVBoxLayout *rbox = new QVBoxLayout;
    rbox->addWidget(m_reversechanges);
    rbox->addWidget(m_filterslabel);
    rbox->addWidget(m_filters);
    m_reversegroup->setLayout(rbox);
    m_midlayout->addWidget(m_reversegroup);

    m_midlayout->addStretch();

    m_progress = new QProgressBar;
    m_midlayout->addWidget(m_progress);

    m_syllabify = new QLineEdit;
    m_midlayout->addWidget(m_syllabify);

    m_syllableseperatorlabel = new QLabel("Syllable seperator: ");
    m_syllableseperatorlayout->addWidget(m_syllableseperatorlabel);

    m_syllableseperator = new QLineEdit;
    m_syllableseperator->setMaxLength(1);
    // make width equal to 1em so it's clear the textbox only accepts 1 char
    // we multiply by 2 because we also need some space on the side
    m_syllableseperator->setFixedWidth(m_syllableseperator->fontMetrics().width('M') * 2);
    m_syllableseperator->setText("-");
    m_syllableseperator->setAlignment(Qt::AlignCenter);
    m_syllableseperatorlayout->addWidget(m_syllableseperator);

    m_syllableseperatorlayout->addStretch();
    m_syllableseperatorlayout->setSpacing(0);

    m_midlayout->addLayout(m_syllableseperatorlayout);
    m_layout->addLayout(m_midlayout);

    m_results = new QTextEdit;
    m_results->setReadOnly(true);
    m_layout->addWidget(m_results);

    m_categorieslist = new QMap<QChar, QList<QChar>>();

    connect(m_categories, &QPlainTextEdit::textChanged, this, &Window::UpdateCategories);
    connect(m_apply, &QPushButton::clicked, this, &Window::DoSoundChanges);

    fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Open .esc file", this, &Window::OpenEsc);
    fileMenu->addAction("Open .lex file", this, &Window::OpenLex);
    fileMenu->addAction("Save .esc file", this, &Window::SaveEsc);
    fileMenu->addAction("Save .lex file", this, &Window::SaveLex);

    toolsMenu = menuBar()->addMenu("Tools");
    toolsMenu->addAction("Affixer", this, &Window::LaunchAffixer);

    helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About", this, &Window::LaunchAboutBox);
    helpMenu->addAction("About Qt", this, &Window::LaunchAboutQt);

    if (QCoreApplication::arguments().length() > 1)
    {
        RealOpenEsc(QCoreApplication::arguments().at(1));
    }
}

void Window::DoSoundChanges()
{
    bool reverse = m_reversechanges->isChecked();
    QStringList changes = ApplyRewrite(m_rules->toPlainText()).split('\n', QString::SkipEmptyParts);
    if (reverse) std::reverse(changes.begin(), changes.end());

    m_progress->setMaximum(qMax(1, changes.length()));    // we use qMax to avoid showing a busy indicator when no changes are specified
    m_progress->setMinimum(0);
    m_progress->setValue(0);

    QStringList result;
    QString syllabifyregexp(SoundChanges::PreProcessRegexp(m_syllabify->text(), *m_categorieslist));
    QString report;
    for (QString word : m_words->toPlainText().split('\n'))
    {
        QString gloss = "";
        bool hasGloss = false;

        if (word.split('>').length() > 1)
        {
            hasGloss = true;
            QStringList split = word.split('>');
            gloss = split.at(1);
            word = split.at(0);
        }

        QString changed = "";
        for (QString subword : word.split(' ', QString::SkipEmptyParts))
        {
            QStringList subchanged = ApplyRewrite(subword).split(' ', QString::SkipEmptyParts);

            for (QString change : changes)
            {
                bool alwaysApply = false;
                for (QString &_subchanged : subchanged)
                {
                    QStringList splitchange = change.replace(QRegularExpression(R"(\*.*)"), "").split(' ', QString::SkipEmptyParts);
                    if (splitchange.length() == 0) continue;
                    QString _change;
                    int prob = 100;
                    if (splitchange.length() > 1)
                    {
                        _change = splitchange.at(1);
                        switch (splitchange.at(0).at(0).toLatin1())
                        {
                        case 'x':
                            _subchanged = SoundChanges::Syllabify(syllabifyregexp, _subchanged, m_syllableseperator->text().at(0));
                        case '?':
                        {
                            bool ok;
                            int _prob = QString(splitchange.at(0).mid(1)).toInt(&ok);
                            if (ok)
                            {
                                prob = _prob;
                            }
                        }
                        case 'f':
                            if (reverse)
                                goto CONTINUE;
                        case 'a':
                            alwaysApply = true;
                        }
                    }
                    else _change = splitchange.at(0);
                    {
                        QString before = _subchanged;
                        _subchanged = SoundChanges::RemoveDuplicates(SoundChanges::ApplyChange(_subchanged, _change, *m_categorieslist, prob, reverse, alwaysApply).join(' '));
                        _subchanged.remove(m_syllableseperator->text().at(0));
                        if (_subchanged != before)
                            report.append(QString("<b>%1</b> changed <b>%2</b> to <b>%3</b><br/>").arg(_change, before, _subchanged));
                    }
                CONTINUE:;
                }
                subchanged = SoundChanges::Reanalyse(subchanged);
                m_progress->setValue(m_progress->value() + 1);
            }
            m_progress->setValue(0);

            QString subchangedJoined = SoundChanges::Filter(subchanged, m_filters->toPlainText().split('\n', QString::SkipEmptyParts), *m_categorieslist).join(' ');
            if (m_doBackwards->isChecked()) subchangedJoined = ApplyRewrite(subchangedJoined, true);
            if (m_showChangedWords->isChecked() && subchangedJoined != subword) subchangedJoined = QString("<b>").append(subchangedJoined).append("</b>");

            if (changed.length() == 0) changed =        subchangedJoined;
            else                       changed += ' ' + subchangedJoined;
        }
        changed = FormatOutput(word.trimmed(), changed.trimmed(), gloss.trimmed(), hasGloss);
        result.append(changed);
    }
    m_results->setHtml(result.join("<br/>"));

    if (m_reportChanges->isChecked())
    {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->setText(report);
        msgBox->setWindowModality(Qt::NonModal);
        msgBox->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        msgBox->show();
    }
}

void Window::UpdateCategories()
{
    bool first = true;
    for (QString line : ApplyRewrite(m_categories->toPlainText()).split('\n', QString::SkipEmptyParts))
    {
        if (!QRegularExpression("^.=.+$").match(line).hasMatch()) continue;
        QStringList parts = line.split("=");

        if (first) { m_categorieslist = new QMap<QChar, QList<QChar>>; first = false; }
        QList<QChar> phonemes;
        for (QChar c : parts.at(1))
        {
            if (m_categorieslist->contains(c)) phonemes.append(m_categorieslist->value(c));
            else phonemes.append(c);
        }
        m_categorieslist->insert(parts.at(0).at(0), phonemes);
    }

    QString regexp("");
    first = true;
    for (QChar key : m_categorieslist->keys())
    {
        if (!first) regexp.append("|");
        if (first) first = false;
        regexp.append(key);
    }
    m_highlighter->MakeHighlightingRules(regexp);
    m_highlighter->rehighlight();
}

QString Window::ApplyRewrite(QString str, bool backwards)
{
    QString rewritten = str;
    QString s = m_rewrites->toPlainText();
    for (QString line : m_rewrites->toPlainText().split('\n', QString::SkipEmptyParts))
    {
        QStringList parts = line.split('>');
        if (parts.length() != 2) continue;
        if (backwards) rewritten.replace(parts.at(1), parts.at(0));
        else           rewritten.replace(parts.at(0), parts.at(1));
    }
    return rewritten;
}

QString Window::FormatOutput(QString in, QString out, QString gloss, bool hasGloss)
{
    const QChar arrow(0x2192);
    if (m_plainformat->isChecked())
    {
        if (hasGloss) return QString("%1 > %2").arg(out, gloss);
        else         return out;
    }
    else if (m_arrowformat->isChecked())
    {
        return QString("%1 %2 %3").arg(in, arrow, out);
    }
    else if (m_squareinputformat->isChecked())
    {
        return QString("%1 [%2]").arg(out, in);
    }
    else if (m_squareglossformat->isChecked())
    {
        if (hasGloss) return QString("%1 [%2]").arg(out, gloss);
        else         return out;
    }
    else if (m_arrowglossformat->isChecked())
    {
        if (hasGloss) return QString("%1 %2 %3 [%4]").arg(in, arrow, out, gloss);
        else         return QString("%1 %2 %3").arg(in, arrow, out);
    }
    return out;
}

void Window::LaunchAffixer()
{
    AffixerDialog *affixer = new AffixerDialog;
    affixer->show();
    connect(affixer, &AffixerDialog::addText, this, &Window::AddFromAffixer);
}

void Window::AddFromAffixer(QStringList words, AffixerDialog::PlaceToAdd placeToAdd)
{
    QString textToAdd = words.join('\n');
    switch (placeToAdd)
    {
    case AffixerDialog::PlaceToAdd::AddToStart:
        m_words->setPlainText(textToAdd.append('\n').append(m_words->toPlainText()));
        break;
    case AffixerDialog::PlaceToAdd::AddToEnd:
        m_words->setPlainText(m_words->toPlainText().append('\n').append(textToAdd));
        break;
    case AffixerDialog::PlaceToAdd::AddAtCursor:
        m_words->insertPlainText(QString('\n').append(textToAdd).append('\n'));
        break;
    case AffixerDialog::PlaceToAdd::Overwrite:
        m_words->setPlainText(textToAdd);
        break;
    }
}

void Window::OpenEsc()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open .esc File", QString(), "exSCA Files (*.esc);;All files (*.*)");
    RealOpenEsc(fileName);
}

void Window::RealOpenEsc(QString fileName)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Could Not Open File", "The file could not be opened");
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");

    QStringList cats, rules, rews;

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (line.contains('=')) cats.append(line);
        else if (line.contains('>') && !line.contains('/')) rews.append(line);
        else rules.append(line);
    }

    m_categories->setPlainText(cats.join('\n'));
    m_rules->setPlainText(rules.join('\n'));
    m_rewrites->setPlainText(rews.join('\n'));

    file.close();
}

void Window::OpenLex()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open .lex File", QString(), "exSCA word files (*.lex);;All files (*.*)");
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Could Not Open File", "The file could not be opened");
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");

    QStringList words;
    while (!in.atEnd()) words.append(in.readLine());

    m_words->setPlainText(words.join('\n'));

    file.close();
}

void Window::SaveEsc()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save As .esc File", QString(), "exSCA Files (*.esc);;All files (*.*)");
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Could Not Open File", "The file could not be opened");
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << m_categories->toPlainText().toUtf8() << endl;
    out << m_rewrites  ->toPlainText().toUtf8() << endl;
    out << m_rules     ->toPlainText().toUtf8() << endl;

    file.close();
}

void Window::SaveLex()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save As .lex File", QString(), "exSCA word files (*.lex);;All files (*.*)");
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Could Not Open File", "The file could not be opened");
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    for (QString word : m_words->toPlainText().split('\n'))
    {
        out << word.toUtf8() << endl;
    }

    file.close();
}

void Window::LaunchAboutBox()
{
    QMessageBox::about(this, "About exSCA", "<b>exSCA</b><br/>Version 2.0.3<br/>Copyright &copy; Brad Neimann 2017");
}

void Window::LaunchAboutQt()
{
    QMessageBox::aboutQt(this, "About Qt");
}