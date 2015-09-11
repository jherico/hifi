//
//  WindowScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WindowScriptingInterface_h
#define hifi_WindowScriptingInterface_h

#include <QObject>

#include <QString>
#include <QFileDialog>
#include <QComboBox>
#include <QLineEdit>

#include "WebWindowClass.h"

class WindowScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(int innerWidth READ getInnerWidth)
    Q_PROPERTY(int innerHeight READ getInnerHeight)
    Q_PROPERTY(int x READ getX)
    Q_PROPERTY(int y READ getY)
    Q_PROPERTY(bool cursorVisible READ isCursorVisible WRITE setCursorVisible)
public:
    WindowScriptingInterface();
    int getInnerWidth();
    int getInnerHeight();
    int getX();
    int getY();
    bool isCursorVisible() const;

public slots:
    QJSValue getCursorPositionX();
    QJSValue getCursorPositionY();
    void setCursorPosition(int x, int y);
    void setCursorVisible(bool visible);
    QJSValue hasFocus();
    void setFocus();
    void raiseMainWindow();
    QJSValue alert(const QString& message = "");
    QJSValue confirm(const QString& message = "");
    QJSValue form(const QString& title, QJSValue array);
    QJSValue prompt(const QString& message = "", const QString& defaultText = "");
    QJSValue browse(const QString& title = "", const QString& directory = "",  const QString& nameFilter = "");
    QJSValue save(const QString& title = "", const QString& directory = "",  const QString& nameFilter = "");
    QJSValue s3Browse(const QString& nameFilter = "");

    void nonBlockingForm(const QString& title, QJSValue array);
    void reloadNonBlockingForm(QJSValue array);
    QJSValue getNonBlockingFormResult(QJSValue array);
    QJSValue peekNonBlockingFormResult(QJSValue array);

signals:
    void domainChanged(const QString& domainHostname);
    void inlineButtonClicked(const QString& name);
    void nonBlockingFormClosed();
    void svoImportRequested(const QString& url);
    void domainConnectionRefused(const QString& reason);

private slots:
    QJSValue showAlert(const QString& message);
    QJSValue showConfirm(const QString& message);
    QJSValue showForm(const QString& title, QJSValue form);
    QJSValue showPrompt(const QString& message, const QString& defaultText);
    QJSValue showBrowse(const QString& title, const QString& directory, const QString& nameFilter,
                            QFileDialog::AcceptMode acceptMode = QFileDialog::AcceptOpen);
    QJSValue showS3Browse(const QString& nameFilter);

    void showNonBlockingForm(const QString& title, QJSValue array);
    void doReloadNonBlockingForm(QJSValue array);
    bool nonBlockingFormActive();
    QJSValue doGetNonBlockingFormResult(QJSValue array);
    QJSValue doPeekNonBlockingFormResult(QJSValue array);

    void chooseDirectory();
    void inlineButtonClicked();

    void nonBlockingFormAccepted() { _nonBlockingFormActive = false; _formResult = QDialog::Accepted; emit nonBlockingFormClosed(); }
    void nonBlockingFormRejected() { _nonBlockingFormActive = false; _formResult = QDialog::Rejected; emit nonBlockingFormClosed(); }

    WebWindowClass* doCreateWebWindow(const QString& title, const QString& url, int width, int height, bool isToolWindow);
    
private:
    QString jsRegExp2QtRegExp(QString string);
    QDialog* createForm(const QString& title, QJSValue form);
    
    QDialog* _editDialog;
    QJSValue _form;
    bool _nonBlockingFormActive;
    int _formResult;
    QVector<QComboBox*> _combos;
    QVector<QCheckBox*> _checks;
    QVector<QLineEdit*> _edits;
    QVector<QPushButton*> _directories;
};

#endif // hifi_WindowScriptingInterface_h
