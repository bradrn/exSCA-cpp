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

QString SoundChanges::ApplyChange(QString word, QString change, QMap<QChar, QList<QChar>> categories, int probability)
{
    QStringList splitChange = change.split("/");
    QString replaced = word;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rand;                 // used when rule begins with '?'

    for (int wordIndex = 0; wordIndex <= replaced.length(); wordIndex++)  // '<= word.length()' and not '< word.length()' because material can be added to the end of the word (e.g. '/XYZ/_#')
    {
        int startpos, length;
        QQueue<int> catnums;
        bool tryRule = true;
        if (rand(gen) >= (probability / 100.0)) tryRule = false;
        if (tryRule && SoundChanges::TryRule(replaced, wordIndex, change, categories, &startpos, &length, &catnums))
        {
            if (change.at(0) == '_')
            {
                if (splitChange.length() == 2)
                {
                    QRegularExpression regexp = QRegularExpression(PreProcessRegexp(splitChange.at(0).mid(1), categories));
                    replaced = replaced.replace(regexp, splitChange.at(1));
                }
            }
            else
            {
                QString replacement = "";
                QChar lastChar = ' ';
                QList<QChar> backreferences;
                State state = State::Normal;
                QList<QChar> nonceChars;

                for (QChar c : splitChange.at(1))
                {
                    switch (state)
                    {
                    case State::Normal:
                        if (categories.contains(c))
                        {
                            QChar c1;
                            if (catnums.length() > 0) c1 = categories.value(c).at(catnums.dequeue());
                            else c1 = ' ';
                            replacement.append(c1);
                            lastChar = c1;
                            backreferences.append(c1);
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
                        else
                        {
                            replacement.append(c);
                            lastChar = c;
                        }
                        break;
                    case State::Nonce:
                        if (c == ']')
                        {
                            QChar c1 = nonceChars.at(catnums.dequeue());
                            replacement.append(c1);
                            lastChar = c1;
                            backreferences.append(c1);
                            nonceChars = QList<QChar>();
                            state = State::Normal;
                        }
                        else nonceChars.append(c);
                        break;
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
                replaced = replaced.replace(startpos, length, replacement);
                wordIndex += replacement.length() - length;
            }
        }
    }
    return replaced;
}

bool SoundChanges::TryRule(QString word,
                           int wordIndex,
                           QString change,
                           QMap<QChar, QList<QChar>> categories,
                           int *startpos,
                           int *length,
                           QQueue<int> *catnums)
{
    QStringList splitChange = change.split("/");
    if (change.at(0) == '_')
    {
        QRegularExpression regexp = QRegularExpression(PreProcessRegexp(splitChange.at(0).mid(1), categories));
        return regexp.match(word).hasMatch();
    }
    else if (splitChange.length() >= 3)
    {
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
                                 QQueue<int> *outcats,
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
    QQueue<std::pair<QChar, int>> environmentcats;

    if (chars == "_#")
    {
        // THE SPECIAL CASE '/XYZ/_#'
        // The expressions '/XYZ/#_' and '/XYZ/_#' should add XYZ to the beginning and the end of the word respectively.
        // However, they both add to the beginning of the word.
        // This means that the case '/XYZ/_#' needs to be treated seperately from all other cases.

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

            if ((curIndex >= word.length() && c != '#')
             || (curIndex >  word.length() && c == '#')
             ||  curIndex < 0)
                return false;

            if (categories.contains(c))
            {
                for (int i = 0; i < categories.value(c).length(); i++)
                {
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
                bool didAnyApplyBeforeTilde = false;
                bool didAnyApplyAfterTilde = false;
                bool beforeTilde = true;
                for (int i = 0; i < nonceChars.length(); i++)
                {
                    QChar c_nonce = nonceChars.at(i);
                    QChar nonceCharParsed = ' ';
                    if (c_nonce == '~')
                    {
                        beforeTilde = false;
                        if (i == 0) didAnyApplyBeforeTilde = true;
                    }
                    else if (SoundChanges::TryCharacter(word, c_nonce, lastChar, &nonceCharParsed, target, curIndex, categories, 0, 0, 0, recordcats))
                    {
                        if (beforeTilde) didAnyApplyBeforeTilde = true;
                        else             didAnyApplyAfterTilde = true;
                        lastChar = nonceCharParsed;
                        resetPosition = curIndex;
                        if (outcats) outcats->enqueue(i);
                    }
                    curIndex = position;
                }
                doesChangeApply &= (didAnyApplyBeforeTilde && !didAnyApplyAfterTilde);
                curIndex = resetPosition;
                curState = State::Normal;
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
                doesChangeApply &= word.at(curIndex) == categories.value(environmentcats.at(backreference).first)
                                                                  .at(environmentcats.at(backreference).second);
                if (outcats) outcats->enqueue(environmentcats.at(backreference).second);
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
                                QQueue<int> *outcats,
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
    default:
        int catnum;
        if (curIndex >= word.length() || curIndex < 0) return false;
        doesChangeApply &= SoundChanges::MatchChar(word.at(curIndex), c, categories, &catnum);
        if (categories.contains(c) && recordcats && outcats) outcats->enqueue(catnum);
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
