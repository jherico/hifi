//
//  ScriptHighlighting.cpp
//  interface/src
//
//  Created by Thijs Wenker on 4/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptHighlighting.h"

ScriptHighlighting::ScriptHighlighting(QTextDocument* parent) :
    QSyntaxHighlighter(parent)
{
    keywordRegex = QRegExp("\\b(break|case|catch|continue|debugger|default|delete|do|else|finally|for|function|if|in|instanceof|new|return|switch|this|throw|try|typeof|var|void|while|with)\\b");
    qoutedTextRegex = QRegExp("\".*\"");
    multiLineCommentBegin = QRegExp("/\\*");
    multiLineCommentEnd = QRegExp("\\*/");
    numberRegex = QRegExp("[0-9]+(\\.[0-9]+){0,1}");
}

void ScriptHighlighting::highlightBlock(const QString &text) {
    this->highlightKeywords(text);
    this->formatComments(text);	
    this->formatQoutedText(text);
    this->formatNumbers(text);
}

void ScriptHighlighting::highlightKeywords(const QString &text) {
    int index = keywordRegex.indexIn(text);
    while (index >= 0) {
        int length = keywordRegex.matchedLength();
        setFormat(index, length, Qt::blue);
        index = keywordRegex.indexIn(text, index + length);
    }
}

void ScriptHighlighting::formatComments(const QString &text) {

    setCurrentBlockState(BlockStateClean);

    int start = (previousBlockState() != BlockStateInMultiComment) ? text.indexOf(multiLineCommentBegin) : 0;

    while (start > -1) {
        int end = text.indexOf(multiLineCommentEnd, start);
        int length = (end == -1 ? text.length() : (end + multiLineCommentEnd.matchedLength())) - start;
        setFormat(start, length, Qt::lightGray);
        start = text.indexOf(multiLineCommentBegin, start + length);
        if (end == -1) setCurrentBlockState(BlockStateInMultiComment);
    }
}

void ScriptHighlighting::formatQoutedText(const QString &text){
    int index = qoutedTextRegex.indexIn(text);
    while (index >= 0) {
        int length = qoutedTextRegex.matchedLength();
        setFormat(index, length, Qt::red);
        index = qoutedTextRegex.indexIn(text, index + length);
    }
}

void ScriptHighlighting::formatNumbers(const QString &text){
    int index = numberRegex.indexIn(text);
    while (index >= 0) {
        int length = numberRegex.matchedLength();
        setFormat(index, length, Qt::green);
        index = numberRegex.indexIn(text, index + length);
    }
}
