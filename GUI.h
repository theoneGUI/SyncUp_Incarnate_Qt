#pragma once
#define SYNCUP_GUI_MOD
#include <QtWidgets/QMainWindow>
#include "ui_GUI.h"
#include "Settings.h"
#include <QString>
#include <QDialog>
#include "../../../../commons/commons.h"
#include <QSystemTrayIcon>
#include "include/Matchmaker.h"

#ifndef QT_NO_SYSTEMTRAYICON

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

class GUI : public QMainWindow
{
    Q_OBJECT

public:
    GUI(QWidget *parent = nullptr);
    ~GUI();


private:
    void startThread();

private slots:
    void hashedFileCountIncrement(int);
    void prepareProgressBar(int);
    void displayNumErrors(int);
    void onManualScan();
    void hashDirectory();
    void updateView(std::vector<std::string>);
    void changeWatchDir();
    void refreshConfig();

    void selectAll();
    void unselectAll();

    void startFileTransfer();
    void incrementFilesTransferred(int);
    void transferDone();
    void transferFailed(int);

    void beginSUFTP(address);
    void matchFailed(int);

    void SUFTPstatus(const char*);

protected:
    void closeEvent(QCloseEvent*) override;

private:
    QString scan;
    QString store;
    Ui::GUIClass ui;
    ui::SettingsDialog* settings = nullptr;
    std::vector<std::string> results;
    QString strExpectedFileCt;
    int intExpectedFileCt;
    std::vector<QListWidgetItem*> listWidgetItems;

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void showMessage();
    void messageClicked();

private:

    void setupTray();
    bool aleadyTold = false;
    QIcon theIcon;

    QAction* minimizeAction;
    QAction* restoreAction;
    QAction* quitAction;
    QAction* scanAction;

    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;

};

#endif