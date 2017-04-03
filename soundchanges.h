#ifndef SOUNDCHANGES_H
#define SOUNDCHANGES_H


class QString;
class QStringList;
class QChar;
class QRegularExpression;
template <class Key, class T> class QMap;
template <class T> class QList;
template <class T> class QQueue;
namespace std
{
    template <class T1, class T2> struct pair;
}
class SoundChanges
{
public:
    static QStringList ApplyChange(QString word, QString change, QMap<QChar, QList<QChar>> categories, int probability, bool reverse);

    static QString PreProcessRegexp(QString regexp, QMap<QChar,QList<QChar>> categories);

    static QString Syllabify(QString regexp, QString word, QChar seperator);

    static QString RemoveDuplicates(QString s);

    static QStringList Reanalyse(QStringList sl);

private:
    static bool SoundChanges::TryRule(QString word,
                                      int wordIndex,
                                      QString change,
                                      QMap<QChar, QList<QChar>> categories,
                                      int *startpos,
                                      int *length,
                                      QQueue<int> *catnums,
                                      bool reverse);

    static bool TryCharacters(QString word,
                              int wordIndex,
                              int *finalIndex,
                              QString change,
                              QString target,
                              QMap<QChar, QList<QChar>> categories,
                              int *startpos,
                              int *length,
                              QQueue<int> *outcats,
                              bool recordcats,
                              QChar *lastCharParsed);

    static bool TryCharacter(QString word,
                             QChar c,
                             QChar lastChar,
                             QChar *lastCharParsed,
                             QString target,
                             int &curIndex,
                             QMap<QChar, QList<QChar>> categories,
                             int *startpos,
                             int *length,
                             QQueue<int> *outcats,
                             bool recordcats);

    static bool MatchChar(QChar char1,
                          QChar char2,
                          QMap<QChar, QList<QChar>> categories,
                          int *catnum);

    static int ActualLength(QString rule);

    static int MaxLength(QList<std::pair<QString, int>> l);

    static void ReverseFirstTwo(QStringList &l);

    enum class State
    {
        Normal,
        Optional,
        Nonce,
        Backreference
    };
};

#endif // SOUNDCHANGES_H
