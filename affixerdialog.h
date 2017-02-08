#ifndef AFFIXERDIALOG_H
#define AFFIXERDIALOG_H

#include <QDialog>

class QWidget;
class QString;
class QStringList;
class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;
class QPlainTextEdit;
class QLabel;
class QPushButton;

template <class T> class QList;

class AffixerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AffixerDialog(QWidget *parent = 0);

    enum class PlaceToAdd
    {
        AddToStart,
        AddToEnd,
        AddAtCursor,
        Overwrite
    };

signals:
    void addText(QStringList words, PlaceToAdd placeToAdd);

private slots:
    void textEntered();
    void emitAddTextToStart();
    void emitAddTextToEnd();
    void emitAddTextAtCursor();
    void emitOverwriteText();

private:
    QVBoxLayout *m_layout;
    QHBoxLayout *m_bottomlayout;
    QGridLayout *m_toplayout;

    QPlainTextEdit *m_roots;
    QPlainTextEdit *m_prefixes;
    QPlainTextEdit *m_infixes;
    QPlainTextEdit *m_suffixes;
    QPlainTextEdit *m_preview;

    QLabel *m_rootslabel;
    QLabel *m_prefixeslabel;
    QLabel *m_infixeslabel;
    QLabel *m_suffixeslabel;
    QLabel *m_previewlabel;

    QPushButton *m_addToStart;
    QPushButton *m_addToEnd;
    QPushButton *m_addAtCursor;
    QPushButton *m_overwritetext;

    QStringList GetWords();
    QList<QStringList> GetSlotAffixes(QStringList slots);
    QList<QStringList> CartesianProduct(QList<QStringList> sequences);
};

#endif
