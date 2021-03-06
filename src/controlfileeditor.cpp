#include "controlfileeditor.h"
#include "syntaxhighlighter.h"
#include <QTextBlock>
#include <QRegularExpression>

ControlFileEditor::ControlFileEditor(QWidget *parent)
    : CodeEditor(parent)
{
    packageName = "packagename";
    version = "1.0";
    connect(this, SIGNAL(textChanged()), this, SLOT(infoIsEdited()));
    showLineNumber(false);
    showHighlightLine(false);

    setSyntaxHighlighter(new SyntaxHighLighter(SyntaxHighLighter::KEY_VALUE, document()));
}

ControlFileEditor::~ControlFileEditor()
{

}

QString ControlFileEditor::getPackageName() const
{
    return packageName;
}

QString ControlFileEditor::getVersion() const
{
    return version;
}

void ControlFileEditor::setPackageName(const QString &pname)
{
    packageName = pname;
    emit packageNameChanged(packageName);
}

void ControlFileEditor::setVersion(const QString &v)
{
    version = v;
    emit versionChanged(version);
}

void ControlFileEditor::infoIsEdited()
{
    QTextBlock txtBlock = document()->findBlockByNumber(textCursor().blockNumber());
    QString curLine = txtBlock.text();
    QRegularExpression regex("^(?<info>Package|Version):\\s?(?<value>.*)$");
    QRegularExpressionMatch rem = regex.match(curLine);
    if (rem.hasMatch()){
        if ("Package" == rem.captured("info")){
            packageName = rem.captured("value");
            emit packageNameChanged(rem.captured("value"));
        } else {
            version = rem.captured("value");
            emit versionChanged(rem.captured("value"));
        }
    }
}
