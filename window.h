#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include "affixerdialog.h"

class QHBoxLayout;
class QVBoxLayout;
class QGroupBox;
class QPlainTextEdit;
class QTextEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QChar;
class QString;
class QStringList;
class QCheckBox;
class QMenu;
class QRadioButton;
class Highlighter;

template <class Key, class T> class QMap;

template <class T> class QList;

class Window : public QMainWindow
{
    Q_OBJECT
public:
    explicit Window();

private:
    QHBoxLayout *m_layout;
    QVBoxLayout *m_leftlayout;
    QVBoxLayout *m_midlayout;
    QHBoxLayout *m_syllableseperatorlayout;

    QPlainTextEdit *m_categories;
    QPlainTextEdit *m_rewrites;
    QPlainTextEdit *m_rules;
    QPlainTextEdit *m_words;
    QPushButton *m_apply;
    QLineEdit *m_syllabify;
    QLineEdit *m_syllableseperator;
    QLabel *m_syllableseperatorlabel;
    QTextEdit *m_results;

    QCheckBox *m_showChangedWords;
    QCheckBox *m_reportChanges;
    QCheckBox *m_doBackwards;

    QGroupBox *m_formatgroup;
    QRadioButton *m_plainformat;
    QRadioButton *m_arrowformat;
    QRadioButton *m_squareinputformat;
    QRadioButton *m_squareglossformat;
    QRadioButton *m_arrowglossformat;

    Highlighter *m_highlighter;

    QMap<QChar, QList<QChar>> *m_categorieslist;

    QString ApplyRewrite(QString str, bool backwards = false);
    QString FormatOutput(QString in, QString out, QString gloss, bool isGloss);

    QMenu *fileMenu;
    QMenu *toolsMenu;
    QMenu *helpMenu;

private slots:
    void DoSoundChanges();
    void UpdateCategories();
    void AddFromAffixer(QStringList words, AffixerDialog::PlaceToAdd placeToAdd);

    void OpenEsc();
    void OpenLex();
    void SaveEsc();
    void SaveLex();
    void RealOpenEsc(QString fileName);

    void LaunchAffixer();
    void LaunchAboutBox();
    void LaunchAboutQt();
};

#endif // WINDOW_H
