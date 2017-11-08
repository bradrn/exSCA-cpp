#include <algorithm>
#include <utility>
#include <QString>
#include <QStringList>
#include <QChar>
#include <QMap>
#include <QList>
#include <QQueue>
#include <QStack>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <random>
#include "soundchanges.h"

QStringList SoundChanges::ApplyChange(QString word, QString change, QMap<QChar, QList<QChar>> categories, int probability, bool reverse, bool alwaysApply, bool sometimesApply)
{
    QStringList splitChange = change.split("/");
    if (reverse) ReverseFirstTwo(splitChange);
    QList<std::pair<QString, int>> replaced;
    replaced.append(std::make_pair(word, 0));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rand;                 // used when rule begins with '?'

    for (int wordIndex = 0; wordIndex <= MaxLength(replaced); wordIndex++) // '<=' and not '<' because material can be added to the end of the word (e.g. '/XYZ/_#')
    {
        int startpos, length;
        QQueue<std::pair<int, QChar>> catnums;
        bool tryRule = true;
        if (rand(gen) >= (probability / 100.0)) tryRule = false;
        QList<std::pair<QString, int>> newReplaced;
        for (std::pair<QString, int> _replaced : replaced)
        {
            int _wordIndex = _replaced.second;
            if (_wordIndex > _replaced.first.length()) continue;
            if (tryRule && SoundChanges::TryRule(_replaced.first, _wordIndex, change, categories, &startpos, &length, &catnums, reverse))
            {
                if (change.at(0) == '_')
                {
                    if (splitChange.length() == 2)
                    {
                        if (reverse) newReplaced.append(_replaced);  // we can't reverse regexes yet, so we just re-add the original word
                        else
                        {
                            QRegularExpression regexp = QRegularExpression(PreProcessRegexp(splitChange.at(0).mid(1), categories));
                            newReplaced.append(std::make_pair(_replaced.first.replace(regexp, splitChange.at(1)), _replaced.second));
                        }
                    }
                }
                else
                {
                    QStringList replacements("");
                    QStringList newReplacements;
                    QChar lastChar = ' ';
                    QList<QChar> backreferences;
                    State state = State::Normal;
                    QList<QChar> nonceChars;
                    bool insertMultiple = false;

                    for (QChar c : splitChange.at(1))
                    {
                        for (QString &replacement : replacements)
                        {
                            switch (state)
                            {
                            case State::Normal:
                                if (categories.contains(c))
                                {
                                    if ((catnums.length() > 0) && (!insertMultiple))
                                    {
                                        QChar c1;
                                        std::pair<int, QChar> deq = catnums.dequeue();
                                        if (categories.value(c).length() > deq.first) c1 = categories.value(c).at(deq.first);
                                        else c1 = deq.second;

                                        replacement.append(c1);
                                        lastChar = c1;
                                        backreferences.append(c1);
                                    }
                                    else
                                    {
                                        for (QChar c1 : categories.value(c))
                                        {
                                            newReplacements.append(replacement + c1);
                                        }
                                    }
                                    insertMultiple = false;
                                }
                                else if (c == '\\')
                                {
                                    QString wordSegment = word.mid(startpos, length);
                                    for (int i = wordSegment.length() - 1; i >= 0; i--)
                                    {
                                        replacement.append(wordSegment.at(i));
                                    }
                                    lastChar = wordSegment.at(wordSegment.length() - 1);
                                }
                                else if (c == '>')
                                {
                                    replacement.append(lastChar);
                                    // no change in lastChar
                                }
                                else if (c == '~') catnums.dequeue();
                                else if (c == '@') state = State::Backreference;
                                else if (c == '[') state = State::Nonce;
                                else if (c == '`') insertMultiple = true;
                                else
                                {
                                    replacement.append(c);
                                    lastChar = c;
                                }
                                break;
                            case State::Nonce:
                                if (c == ']')
                                {
                                    std::pair<QString, bool> parsedChars = SoundChanges::ParseNonce(nonceChars, categories);
                                    if ((catnums.length() > 0) && (!insertMultiple))
                                    {
                                        QChar c1;
                                        std::pair<int, QChar> deq = catnums.dequeue();
                                        if (parsedChars.first.length() > deq.first) c1 = parsedChars.first.at(deq.first);
                                        else c1 = deq.second;

                                        replacement.append(c1);
                                        lastChar = c1;
                                        backreferences.append(c1);
                                        nonceChars = QList<QChar>();
                                    }
                                    else
                                    {
                                        for (QChar c1 : parsedChars.first)
                                        {
                                            newReplacements.append(replacement + c1);
                                        }
                                    }
                                    state = State::Normal;
                                }
                                else nonceChars.append(c);
                                break;
                            case State::Optional: break;
                            case State::Backreference:
                                bool ok = false;
                                int backreference = QString(c).toInt(&ok) - 1;  // We start at '@1' but this corresponds to index 0 so we subtract 1
                                if (ok)
                                {
                                    replacement.append(backreferences.at(backreference));
                                }
                                state = State::Normal;
                                break;
                            }
                        }

                        if (newReplacements.length() > 0)
                        {
                            replacements = newReplacements;
                            newReplacements = QStringList();
                        }
                    }

                    for (QString replacement : replacements)
                    {
                        // so 'first.replace()' doesn't modify _replaced.first
                        QString first(_replaced.first);
                        QString replaced = first.replace(startpos, length, replacement);
                        newReplaced.append(std::make_pair(replaced, _replaced.second + replacement.length() - length + 1));
                    }
                }
                if ((reverse && !alwaysApply) || sometimesApply) newReplaced.append(std::make_pair(_replaced.first, _replaced.second + 1));
            }
        }

        if (newReplaced.length() == 0)
        {
            // even if we aren't replacing anything, we still need to make sure we add one to the current position
            for (std::pair<QString, int> _replaced : replaced)
            {
                newReplaced.append(std::make_pair(_replaced.first, _replaced.second + 1));
            }
        }
        replaced = newReplaced;
        newReplaced = QList<std::pair<QString, int>>();
    }

    QStringList result;
    for (std::pair<QString, int> _replaced : replaced)
    {
        bool append = true;
        if (reverse)
        {
            QStringList l = SoundChanges::ApplyChange(_replaced.first, change, categories, probability, false, false, false);
            append = (l.length() == 1) && (l.at(0) == word);
        }
        if (append) result.append(_replaced.first);
    }
    return result;
}

bool SoundChanges::TryRule(QString word,
                           int wordIndex,
                           QString change,
                           QMap<QChar, QList<QChar>> categories,
                           int *startpos,
                           int *length,
                           QQueue<std::pair<int, QChar>> *catnums,
                           bool reverse)
{
    QStringList splitChange = change.split("/");
    if (change.at(0) == '_')
    {
        QRegularExpression regexp = QRegularExpression(PreProcessRegexp(splitChange.at(0).mid(1), categories));
        return regexp.match(word).hasMatch();
    }
    else if (splitChange.length() >= 3)
    {
        if (reverse) ReverseFirstTwo(splitChange);
        if (splitChange.length() > 2)
        {
            for (int i = 3; i < splitChange.length(); i++)
            {
                QStringList splitException = splitChange.at(i).split("_");
                int beforePartLength        = SoundChanges::ActualLength(splitException              .at(0));
                int beforeEnvironmentLength = SoundChanges::ActualLength(splitChange.at(2).split("_").at(0));
                if (SoundChanges::TryCharacters(word,
                                                wordIndex + (beforeEnvironmentLength - beforePartLength),
                                                0,
                                                splitException.at(0),
                                                splitChange.at(0),
                                                categories,
                                                0,
                                                0,
                                                0,
                                                false,
                                                0))
                {
                    if (SoundChanges::TryCharacters(word,
                                                    wordIndex + beforeEnvironmentLength,
                                                    0,
                                                    "_" + splitException.at(1),
                                                    splitChange.at(0),
                                                    categories,
                                                    0,
                                                    0,
                                                    0,
                                                    false,
                                                    0))
                    {
                        return false;
                    }
                }
            }
        }

        return SoundChanges::TryCharacters(word,
                                           wordIndex,
                                           0,
                                           splitChange.at(2),
                                           splitChange.at(0),
                                           categories,
                                           startpos,
                                           length,
                                           catnums,
                                           false,
                                           0);
    }
    else return false;
}

bool SoundChanges::TryCharacters(QString word,
                                 int wordIndex,
                                 int *finalIndex,
                                 QString chars,
                                 QString target,
                                 QMap<QChar, QList<QChar>> categories,
                                 int *startpos,
                                 int *length,
                                 QQueue<std::pair<int, QChar>> *outcats,
                                 bool recordcats,
                                 QChar *lastCharParsed)
{
    if (startpos) *startpos = 0;
    if (length)   *length   = 0;
    bool doesChangeApply = true;
    int curIndex = wordIndex;         // Place in word we are currently at

    State curState = State::Normal;
    QStack<std::pair<int, bool>> opt_stateStack;  // For when we are in a state of State::Optional, this records the index we are currently at
                                                  // and the current value of doesChangeApply. When we encounter a '(' we push curIndex and
                                                  // and doesChangeApply, and if the optional part fails we pop them back off.

    QList<QChar> nonceChars;
    QChar lastChar = ' ';
    QQueue<std::pair<QChar, int>> environmentcats;     // Categories encountered so far

    // THE SPECIAL CASES '/XYZ/#_' and '/XYZ/_#'
    // =========================================
    //
    // The expressions '/XYZ/#_' and '/XYZ/_#' should add XYZ to the beginning and the end of the word respectively.
    // However, without special treatment, they both add XYZ to both the beginning and the end of the word, meaning
    // that they have to be treated seperately from all other cases.
    if (chars == "_#")
    {
        doesChangeApply &= SoundChanges::TryCharacter
                                        (word,
                                        '_',
                                        lastChar,
                                        &lastChar,
                                        target,
                                        curIndex,
                                        categories,
                                        startpos,
                                        length,
                                        outcats,
                                        recordcats);
        doesChangeApply &= curIndex == word.length();
        return doesChangeApply;
    }
    else if (chars == "#_")
    {
        doesChangeApply &= curIndex == 0;
        doesChangeApply &= SoundChanges::TryCharacter
                                        (word,
                                        '_',
                                        lastChar,
                                        &lastChar,
                                        target,
                                        curIndex,
                                        categories,
                                        startpos,
                                        length,
                                        outcats,
                                        recordcats);
        return doesChangeApply;
    }

    for (QChar c : chars)
    {
        switch (curState)
        {
        case State::Normal:
            if (c == '(')
            {
                opt_stateStack.push(std::make_pair(curIndex, doesChangeApply));
                curState = State::Optional;
                break;
            }
            else if (c == '[')
            {
                curState = State::Nonce;
                break;
            }
            else if (c == '@')
            {
                curState = State::Backreference;
                break;
            }

            if (categories.contains(c))
            {
                for (int i = 0; i < categories.value(c).length(); i++)
                {
                    if (curIndex < 0 || curIndex >= word.length()) return false;
                    if (word.at(curIndex) == categories.value(c).at(i))
                    {
                        environmentcats.enqueue(std::make_pair(c, i));
                        break;
                    }
                }
            }
            doesChangeApply &= SoundChanges::TryCharacter
                                            (word,
                                             c,
                                             lastChar,
                                             &lastChar,
                                             target,
                                             curIndex,
                                             categories,
                                             startpos,
                                             length,
                                             outcats,
                                             recordcats);
            break;
        case State::Optional:
            if (c == ')')
            {
                if (!doesChangeApply)
                {
                    std::pair<int, bool> prevState = opt_stateStack.pop();
                    curIndex = prevState.first;
                    doesChangeApply = prevState.second;
                }
                if (opt_stateStack.length() == 0) curState = State::Normal;
                break;
            }
            else if (c == '(')
            {
                opt_stateStack.push(std::make_pair(curIndex, doesChangeApply));
                break;
            }
            doesChangeApply &= SoundChanges::TryCharacter
                                            (word,
                                             c,
                                             lastChar,
                                             &lastChar,
                                             target,
                                             curIndex,
                                             categories,
                                             startpos,
                                             length,
                                             outcats,
                                             recordcats);
            break;
        case State::Nonce:
            if (c == ']')
            {
                int position = curIndex;                     // SoundChanges::TryCharacter changes the value of curIndex
                                                             // so if it returns false we can reset it back to position

                int resetPosition = curIndex;                // Index to reset to when done

                bool didAnyApply = false;

                std::pair<QString, bool> parsedChars = SoundChanges::ParseNonce(nonceChars, categories);

                for (int i = 0; i < parsedChars.first.length(); i++)
                {
                    QChar c_nonce = parsedChars.first.at(i);;
                    QChar nonceCharParsed = ' ';
                    QQueue<std::pair<int, QChar>> _outcats;
                    if (SoundChanges::TryCharacter(word, c_nonce, lastChar, &nonceCharParsed, target, curIndex, categories, 0, 0, &_outcats, recordcats))
                    {
                        didAnyApply = true;
                        lastChar = nonceCharParsed;
                        resetPosition = curIndex;
                        if (recordcats && outcats)
                        {
                            if (_outcats.length() > 0) outcats->append(std::make_pair(_outcats.dequeue().first, c_nonce));
                            else                       outcats->enqueue(std::make_pair(i, c_nonce));
                        }
                        break;
                    }
                    curIndex = position;
                }

                if (parsedChars.second) doesChangeApply &= didAnyApply;
                else                    doesChangeApply &= !didAnyApply;
                curIndex = resetPosition;
                curState = State::Normal;
                nonceChars = QList<QChar>();
                break;
            }
            nonceChars.append(c);
            break;
        case State::Backreference:
            bool ok = false;
            int backreference = QString(c).toInt(&ok) - 1;     // We start at '@1' but this corresponds to index 0 so we subtract 1
            if (ok)
            {
                if (curIndex >= word.length() || curIndex < 0) return false;
                if (backreference >= environmentcats.length()) return false;
                QChar c1 = categories.value(environmentcats.at(backreference).first).at(environmentcats.at(backreference).second);
                doesChangeApply &= word.at(curIndex) == c1;
                if (recordcats && outcats) outcats->enqueue(std::make_pair(environmentcats.at(backreference).second, c));
                curIndex++;
            }
            curState = State::Normal;
            break;
        }
    }
    if (finalIndex) *finalIndex = curIndex;
    if (lastCharParsed) *lastCharParsed = lastChar;
    return doesChangeApply;
}

bool SoundChanges::TryCharacter(QString word,
                                QChar c,
                                QChar lastChar,
                                QChar *lastCharParsed,
                                QString target,
                                int &curIndex,
                                QMap<QChar, QList<QChar>> categories,
                                int *startpos,
                                int *length,
                                QQueue<std::pair<int, QChar>> *outcats,
                                bool recordcats)
    // we pass startpos, length, outcats by reference so we can pass a null pointer if we don't need them
{
    bool doesChangeApply = true;

    switch (c.toLatin1())
    {
    case '#':
        doesChangeApply &= (curIndex == 0) || (curIndex == word.length());
        if ((curIndex != 0) && doesChangeApply)     // if we are at the last character
            return true;                            // return immediately to avoid errors
        break;
    case '_':
        if (startpos) *startpos = curIndex;
        doesChangeApply &= SoundChanges::TryCharacters(word, curIndex, &curIndex, target, target, categories, 0, 0, outcats, true, lastCharParsed);
        if (length && startpos) *length = curIndex - *startpos;
        break;
    case '>':
        if (curIndex >= word.length() || curIndex < 0) return false;
        doesChangeApply &= word.at(curIndex) == lastChar;
        *lastCharParsed = lastChar;
        curIndex++;
        break;
    case '~':
        break;          // we ignore tildes in environment and target
    default:
        int catnum;
        if (curIndex >= word.length() || curIndex < 0) return false;
        doesChangeApply &= SoundChanges::MatchChar(word.at(curIndex), c, categories, &catnum);
        if (categories.contains(c) && recordcats && outcats) outcats->enqueue(std::make_pair(catnum, categories.value(c).at(catnum)));
        *lastCharParsed = word.at(curIndex);
        curIndex++;
        break;
    }
    return doesChangeApply;
}

// we pass catnum by reference so we can pass a pointer if we don't need it
bool SoundChanges::MatchChar(QChar char1, QChar char2, QMap<QChar, QList<QChar>> categories, int *catnum)
{
 
    if (catnum) *catnum = 0;
    if (categories.contains(char2))
    {
        for (int i = 0; i < categories.value(char2).length(); i++)
        {
            if (char1 == categories.value(char2).at(i))
            {
                if (catnum) *catnum = i;
                return true;
            }
        }
        return false;
    }
    return char1 == char2;
}

std::pair<QString, bool> SoundChanges::ParseNonce(QList<QChar> nonce, QMap<QChar, QList<QChar>> categories)
{
    QList<QChar> beforeTilde, afterTilde;
    bool hasTildeOcurred = false;

    for (QChar c : nonce)
    {
        switch (c.toLatin1())
        {
        case '~':
            hasTildeOcurred = true;
            break;
        default:
            if (categories.contains(c))
            {
                QList<QChar> cl = categories.value(c);
                for (QChar _c : cl)
                {
                    if (hasTildeOcurred) afterTilde .append(_c);
                    else                 beforeTilde.append(_c);
                }
            }
            else
            {
                if (hasTildeOcurred) afterTilde .append(c);
                else                 beforeTilde.append(c);
            }
        }
    }

    QString result;
    for (QChar c : beforeTilde)
    {
        if (afterTilde.contains(c)) continue;
        result.append(c);
    }

    return std::make_pair(result, beforeTilde.length() != 0);
}

int SoundChanges::ActualLength(QString rule)
{
    int length = 0;
    State curState = State::Normal;

    for (QChar c : rule)
    {
        switch (curState)
        {
        case State::Normal:
            switch (c.toLatin1())
            {
            case '(':
                curState = State::Optional;
                break;
            case '[':
                curState = State::Nonce;
                break;
            case '@':
                curState = State::Backreference;
                break;
            case '#':
                break;
            default:
                length += 1;
                break;
            }
        case State::Optional:
            if (c == ')')
            {
                curState = State::Normal;
                break;
            }
            // we shouldn't have an optional part before an '_' in a rule exception, so we just do nothing
            break;
        case State::Nonce:
            if (c == ']')
            {
                length += 1;
                curState = State::Normal;
                break;
            }
            break;
        case State::Backreference:
            length += 1;
            break;
        }
    }
    return length;
}

int SoundChanges::MaxLength(QList<std::pair<QString, int>> l)
{
    int maxLength = 0;
    for (std::pair<QString, int> s : l)
    {
        int length = s.first.length();
        if (length > maxLength) maxLength = length;
    }
    return maxLength;
}

void SoundChanges::ReverseFirstTwo(QStringList &l)
{
    if (l.length() >= 2)
    {
        QString t = l.at(0);
        l[0] = l.at(1);
        l[1] = t;
    }
}

QString SoundChanges::PreProcessRegexp(QString regexp, QMap<QChar, QList<QChar>> categories)
{
    QString result = "";
    for (QChar c : regexp)
    {
        if (categories.contains(c))
        {
            result.append('[');
            for (QChar d : categories.value(c))
            {
                result.append(d);
            }
            result.append(']');
        }
        else result.append(c);
    }
    return result;
}

QString SoundChanges::Syllabify(QString regexp, QString word, QChar seperator)
{
    QString result = "";
    QRegularExpression _regexp('^' + regexp);
    QRegularExpressionMatch match = _regexp.match(word);
    bool first = true;
    while (match.hasMatch())
    {
        result.append((first ? "" : QString(seperator)) + match.captured());
        word.remove(_regexp);
        if (first) first = false;
        match = _regexp.match(word);
    }
    return result;
}

QString SoundChanges::RemoveDuplicates(QString s)
{
    QStringList sl;
    for (QString _s : s.split(' '))
    {
        if (!sl.contains(_s))
        {
            sl.append(_s);
        }
    }
    return sl.join(' ');
}

QStringList SoundChanges::Reanalyse(QStringList sl)
{
    QStringList result;
    for (QString s : sl)
    {
        for (QString _s : s.split(' ', QString::SkipEmptyParts))
        {
            if (!result.contains(_s)) result.append(_s);
        }
    }
    return result;
}

QStringList SoundChanges::Filter(QStringList sl, QStringList f, QMap<QChar, QList<QChar>> cats)
{
    QStringList result;
    for (QString s : sl)
    {
        bool append = false;
        for (QString regexp : f)
        {
            append |= QRegularExpression(SoundChanges::PreProcessRegexp(regexp, cats)).match(s).hasMatch();
        }
        if (!append) result.append(s);
    }
    return result;
}
