#include "syntaxhighlighter.h"
#include <QRegularExpressionMatchIterator>

SyntaxHighLighter::SyntaxHighLighter(SyntaxType syntax, QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    switch (syntax) {
    case KEY_VALUE:
        initSyntaxKeyValue();
        break;
    case SECTION_VALUES:
        initSyntaxSectionValues();
        break;
    case SCRIPT:
        initSyntaxScript();
        break;
    default:
        break;
    }
}

SyntaxHighLighter::~SyntaxHighLighter()
{
    highlightingRules.clear();
}

void SyntaxHighLighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

void SyntaxHighLighter::initSyntaxKeyValue()
{
    HighlightingRule rule;

    QTextCharFormat format;

    // key
    format.setForeground(Qt::darkGreen);
    format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^\\w+:");
    rule.format = format;
    highlightingRules.append(rule);

    // comment
    format.setForeground(Qt::gray);
    format.setFontWeight(QFont::Normal);
    rule.pattern = QRegularExpression("^#.*$");
    rule.format = format;
    highlightingRules.append(rule);
}

void SyntaxHighLighter::initSyntaxSectionValues()
{
    HighlightingRule rule;

    QTextCharFormat format;

    // section
    format.setForeground(Qt::darkRed);
    format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^\\[[\\w\\s]+\\]$");
    rule.format = format;
    highlightingRules.append(rule);

    // key
    format.setForeground(Qt::darkGreen);
    format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^\\w+");
    rule.format = format;
    highlightingRules.append(rule);

    // comment
    format.setForeground(Qt::gray);
    format.setFontWeight(QFont::Normal);
    rule.pattern = QRegularExpression("^#.*$");
    rule.format = format;
    highlightingRules.append(rule);
}

void SyntaxHighLighter::initSyntaxScript()
{
    // FIXME: contribute by improving this syntaxic coloration
    HighlightingRule rule;

    QTextCharFormat format;

    // commands list
    format.setForeground(Qt::darkBlue);
    format.setFontWeight(QFont::Bold);
    QStringList commandList;
        commandList << "ls" << "cd" << "mv"
                        << "mkdir" << "rmdir" << "touch"
                        << "rm" << "locate" << "clear"
                        << "echo";
    foreach (const QString &pattern, commandList) {
        rule.pattern = QRegularExpression("\\b"+pattern+"\\b");
        rule.format = format;
        highlightingRules.append(rule);
    }

    // shebang
    format.setForeground(Qt::darkMagenta);
    format.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^#!.*$");
    rule.format = format;
    highlightingRules.append(rule);

    // comment
    format.setForeground(Qt::gray);
    format.setFontWeight(QFont::Normal);
    rule.pattern = QRegularExpression("^#[^!].*$");
    rule.format = format;
    highlightingRules.append(rule);
}
