//
//  UndoStackScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Ryan Huffman on 10/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UndoStackScriptingInterface_h
#define hifi_UndoStackScriptingInterface_h

#include <QUndoCommand>
#include <QUndoStack>


class UndoStackScriptingInterface : public QObject {
    Q_OBJECT
public:
    UndoStackScriptingInterface(QUndoStack* undoStack);

public slots:
    void pushCommand(QJSValue undoFunction, QJSValue undoData, QJSValue redoFunction, QJSValue redoData);

private:
    QUndoStack* _undoStack;
};

class ScriptUndoCommand : public QObject, public QUndoCommand {
    Q_OBJECT
public:
    ScriptUndoCommand(QJSValue undoFunction, QJSValue undoData, QJSValue redoFunction, QJSValue redoData);

    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand* command) { return false; }
    virtual int id() const { return -1; }

public slots:
    void doUndo();
    void doRedo();

private:
    bool _hasRedone;
    QJSValue _undoFunction;
    QJSValue _undoData;
    QJSValue _redoFunction;
    QJSValue _redoData;
};

#endif // hifi_UndoStackScriptingInterface_h
