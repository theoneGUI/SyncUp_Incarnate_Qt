#pragma once

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QFileDialog>
#include <QMessageBox>
#include <string>
#include "include/Traversal.h"
#include "commons.h"


namespace ui
{
    class SettingsDialog : public QDialog
    {
        Q_OBJECT
            QDialogButtonBox* buttonBox;
        QLabel* watchLblTxt;
        QLabel* watchLbl;
        QToolButton* changFolderBtn;

    protected:
        void initDialog(QWidget* parent){
            this->setObjectName("Settings");
            this->resize(562, 242);
            this->setModal(true);
            buttonBox = new QDialogButtonBox(this);
            buttonBox->setObjectName("buttonBox");
            buttonBox->setGeometry(QRect(390, 210, 156, 24));
            buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
            watchLblTxt = new QLabel(this);
            watchLblTxt->setObjectName("watchLblTxt");
            watchLblTxt->setText("Watching folder:");
            watchLblTxt->setGeometry(QRect(50, 40, 91, 16));
            watchLbl = new QLabel(this);
            watchLbl->setObjectName("watchLbl");
            watchLbl->setText(scan);
            watchLbl->setGeometry(QRect(150, 40, 331, 16));
            changFolderBtn = new QToolButton(this);
            changFolderBtn->setObjectName("changFolderBtn");
            changFolderBtn->setText("Change watching folder");
            changFolderBtn->setGeometry(QRect(50, 80, 181, 61));

            connect(changFolderBtn, &QToolButton::released, this, &SettingsDialog::changeFolder);
            connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
            connect(buttonBox, SIGNAL(accepted()), this, SLOT(writeChanges()));
            QMetaObject::connectSlotsByName(this);
        };
    public:
        SettingsDialog(QWidget* parent)
            : QDialog(parent)
        {
            QString tmp;
            tmp = get_env_var("TMP").c_str();

            std::string configPath = get_env_var("APPDATA") + "\\SyncUp\\";

            Traversal config = readConfigFile(configPath + "SU_Config.conf");
            scan = config["scan"].c_str();
            store = config["store"].c_str();
            initDialog(parent);
        }

    public slots:
        void changeFolder() {
            QString diag = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                scan,
                QFileDialog::ShowDirsOnly
                | QFileDialog::DontResolveSymlinks);
            //QMessageBox::information(this, tr("You chose"), diag);
            this->watchLbl->setText(diag);
            if (diag != "")
            {
                scan = diag;
                updated = { std::pair("scan", diag.toStdString()) };
            }
        }

        void writeChanges() {
            writeConfigFile(updated, configWriter::DEFAULT_PATH, configWriter::UPDATE);
            emit triggerRefreshConfig();
            close();
        }

    signals:
        void triggerRefreshConfig();
    private:
        QString scan;
        QString store;
        std::vector<std::pair<std::string, std::string>> updated;
    };
}

