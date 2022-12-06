#define TESTING

#include "GUI.h"
#include "Hasher.h"
#include "include/SUFTP.h"
#include <fstream>
#include <string>
#include <stdlib.h>
#include <QMessageBox>
#include <QThread>
#include <regex>
#include "Worker.h"
#include <fstream>
#include <vector>
#include <utility>
#include <boost/filesystem.hpp>
#include <QCloseEvent>
#include <QSystemTrayIcon>

using std::string;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::pair;
using namespace boost;



GUI::GUI(QWidget *parent) : QMainWindow(parent)
{
    ui.setupUi(this);
    theIcon = QIcon(":/GUI/resources/SyncUp.png");

    setupTray();

    connect(ui.actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(ui.actionSettings, &QAction::triggered, this, &GUI::changeWatchDir);
    connect(ui.manualScan, SIGNAL (released()), this, SLOT (onManualScan()));

    connect(ui.selectAllBtn, &QAbstractButton::released, this, &GUI::selectAll);
    connect(ui.unselectAllBtn, &QAbstractButton::released, this, &GUI::unselectAll);


    this->setWindowIcon(theIcon);
    QString tmp;
    tmp = paths::APPDATA.c_str();

    string configPath = paths::APPDATA + "\\SyncUp\\";

    if (!filesystem::exists(filesystem::path(configPath))) {
        filesystem::create_directory(configPath);
        vector<pair<string, string>> vectorOfPairs{ {pair("scan", paths::USERPROFILE+"\\Desktop\\")}, pair("store", paths::TMP + "\\SU_Hashes.dat")};
        writeConfigFile(vectorOfPairs, configWriter::DEFAULT_PATH, configWriter::OVERWRITE);
    }
    refreshConfig();
}

void GUI::onManualScan() {
    if (!this->isVisible())
        show();
    bool crit = false;
    try {
        Hasher verifyItWorksBeforeCrashing(scan, store);
        startThread();
    }
    catch (std::bad_exception) {
        QMessageBox::critical(this, tr("Watch directory not found!"),
            tr("The folder we're supposed to be watching for changes in files either doesn't exist, or we don't have access to it. Cannot continue!"),
            QMessageBox::Ok);
        crit = true;
    }
    catch (std::exception) {
        ofstream create;
        create.open(store.toStdString());
        create << "ASSOC\n[]";
        create.close();

        startThread();
    }

    if (crit)
        return;

    
}

void GUI::startThread() {
    ui.listWidget->clear();
    listWidgetItems.clear();
    ui.manualScan->setEnabled(false);
    ui.progressBar->setMaximum(0);
    ui.progressBar->setMinimum(0);
    ui.statusTxtLbl->setText("Discovering files...");
    WorkerThread* w = new WorkerThread(scan, store);
    connect(w, &WorkerThread::hashedFileCountIncrement, this, &GUI::hashedFileCountIncrement);
    connect(w, &WorkerThread::totalFiles, this, &GUI::prepareProgressBar);
    connect(w, &WorkerThread::fileError, this, &GUI::displayNumErrors);
    connect(w, &WorkerThread::changedFilesReady, this, &GUI::updateView);
    connect(w, &WorkerThread::finished, w, &QObject::deleteLater);
    w->start();
}

void GUI::prepareProgressBar(int total) {
    QString totalTmp = std::to_string(total).c_str();
    strExpectedFileCt = totalTmp;
    totalTmp = "0/" + totalTmp;
    ui.progressBar->setMaximum(total);
    ui.progressBar->setMinimum(0);
    ui.statusTxtLbl->setText("Processing...");
    ui.countLblProgress->setText(totalTmp);
}

void GUI::hashedFileCountIncrement(int c) {
    QString tmp = std::to_string(c).c_str();
    QString evalTmp = tmp + "/" + strExpectedFileCt;
    ui.progressBar->setValue(c);
    ui.countLblProgress->setText(evalTmp);
}

void GUI::displayNumErrors(int e) {
    QString tmp = std::to_string(e).c_str();
    ui.errLabelCounter->setText(tmp);
}

void GUI::hashDirectory()
{

}

void GUI::updateView(std::vector<std::string> results)
{
    ui.statusTxtLbl->setText("Done");
    ui.progressBar->setValue(0);
    ui.countLblProgress->setText(strExpectedFileCt + "/" + strExpectedFileCt);
    this->results = results;
    for (auto& i : results) {
        std::regex basedir_re(scan.toStdString());
        string tmp = std::regex_replace(i, basedir_re, "");
        //auto& tmp = i;

        QListWidgetItem* tmpItem = new QListWidgetItem(tr(tmp.c_str()), ui.listWidget);
        tmpItem->setCheckState(Qt::Checked);
        listWidgetItems.push_back(tmpItem);
    }
    QString number = std::to_string(results.size()).c_str();
    QString msg = "We found " + number + " changed files.";
    QMessageBox::information(this, tr("Success"), msg, QMessageBox::Ok);
    ui.manualScan->setEnabled(true);
}

void GUI::refreshConfig() {
    Traversal config = readConfigFile(configWriter::DEFAULT_PATH);
    scan = config["scan"].c_str();
    store = config["store"].c_str();
}

void GUI::closeEvent(QCloseEvent* event)
{
    if (!event->spontaneous() || !isVisible())
        return;
    if (trayIcon->isVisible() && !this->aleadyTold) {
        QMessageBox::information(this, tr("SyncUp"),
            tr("SyncUp will keep running in the "
                "system tray. To terminate the program, "
                "right click the SyncUp icon and select <b>Quit</b>."));
        hide();
        event->ignore();
        this->aleadyTold = true;
    }
}

void GUI::showMessage()
{
    QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon(-1);
    trayIcon->showMessage(tr("Hello from SyncUp"), tr("Details"), theIcon, 5000);
}

void GUI::messageClicked()
{
    QMessageBox::information(nullptr, tr("Systray"),
        tr("Sorry, I already gave what help I could.\n"
            "Maybe you should try asking a human?"));
}

void GUI::setupTray()
{
    // creating right click menu on tray icon
    trayIcon = new QSystemTrayIcon(this);

    scanAction = new QAction(tr("Scan now"), this);
    connect(scanAction, &QAction::triggered, this, &GUI::onManualScan);

    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    // connect the dots

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(scanAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    
    trayIcon->setContextMenu(trayIconMenu);

    // connecting functionality
    connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &GUI::messageClicked);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &GUI::iconActivated);

    trayIcon->setIcon(theIcon);
    trayIcon->setToolTip(tr("SyncUp"));
    trayIcon->show();
}

void GUI::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        this->show();
        break;
    case QSystemTrayIcon::MiddleClick:
        showMessage();
        break;
    default:
        ;
    }
}

void GUI::changeWatchDir() {
    settings = new ui::SettingsDialog(this);
    connect(settings, &ui::SettingsDialog::triggerRefreshConfig, this, &GUI::refreshConfig);
    settings->setModal(true);
    settings->show();
}

void GUI::selectAll() {
    for (auto& i : listWidgetItems) {
        i->setCheckState(Qt::Checked);
    }
}

void GUI::unselectAll() {
    for (auto& i : listWidgetItems) {
        i->setCheckState(Qt::Unchecked);
    }
}

GUI::~GUI()
{
}