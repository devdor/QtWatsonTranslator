#include <QDebug>
#include <QMessageBox>
#include <QHttpMultiPart>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "app_settings_dialog.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName("Acme");
    QCoreApplication::setOrganizationDomain("local");
    QCoreApplication::setApplicationName("WatsonTranslatorClient");

    ui->setupUi(this);
    this->createActions();
    this->createMenus();

    QStringList colHeaderList;
    colHeaderList << "Created" << "Text" << "Translation";
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderLabels(colHeaderList);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createActions()
{
    this->appNewAct = new QAction(tr("&New"), this);
    this->appNewAct->setShortcuts(QKeySequence::New);
    this->appNewAct->setStatusTip(tr("Post a new request"));
    this->appNewAct->setIcon(QIcon::fromTheme("document-new"));
    connect(this->appNewAct, &QAction::triggered, this, &MainWindow::fileAppNew);

    this->appSettingsAct = new QAction(tr("&Settings"), this);
    this->appSettingsAct->setStatusTip(tr("Application settings"));
    connect(this->appSettingsAct, &QAction::triggered, this, &MainWindow::fileAppSettings);

    this->appExitAct = new QAction(tr("&Exit"), this);
    this->appExitAct->setShortcut(QKeySequence::Close);
    this->appExitAct->setStatusTip(tr("Exit application"));
    connect(this->appExitAct, &QAction::triggered, this, &MainWindow::fileAppExit);

    this->appAboutAct = new QAction(tr("&About..."), this);
    this->appAboutAct->setStatusTip(tr("About this application"));
    connect(this->appAboutAct, &QAction::triggered, this, &MainWindow::fileAppAbout);
}

void MainWindow::createMenus()
{
    this->fileMenu = this->menuBar()->addMenu(tr("&File"));
    this->fileMenu->addAction(this->appNewAct);
    this->fileMenu->addAction(this->appSettingsAct);
    this->fileMenu->addAction(this->appExitAct);

    this->helpMenu = this->menuBar()->addMenu(tr("Help"));
    this->helpMenu->addAction(this->appAboutAct);

    ui->mainToolBar->addAction(this->appNewAct);
}

void MainWindow::fileAppSettings()
{
    try
    {
        QSettings settings;

        AppSettingsDialog *appSettingsDlg = new AppSettingsDialog(this);
        appSettingsDlg->setWindowFlag(Qt::WindowType::Tool);
        appSettingsDlg->setWindowTitle(tr("Login"));
        appSettingsDlg->Init(
                    settings.value("IBMSettings/ServiceUrl").toString(),
                    settings.value("IBMSettings/VersionDate").toString(),
                    settings.value("IBMSettings/AccessToken").toString());

        int dlgResult = appSettingsDlg->exec();
        if(dlgResult == QDialog::Accepted)
        {
            settings.setValue("IBMSettings/ServiceUrl", appSettingsDlg->GetServiceUrl());
            settings.setValue("IBMSettings/VersionDate", appSettingsDlg->GetVersionDate());
            settings.setValue("IBMSettings/AccessToken", appSettingsDlg->GetAccessToken());
            qDebug() << "appSettingsDlg accepted";
        }
    }
    catch (const std::exception& exc)
    {
        qDebug() << "EXCEPTION in fileAppSettings: " << exc.what() << endl;
    }
}

void MainWindow::fileAppExit()
{
    try
    {
        QApplication::exit();
    }
    catch (const std::exception& exc)
    {
        qDebug() << "EXCEPTION in fileAppExit: " << exc.what() << endl;
    }
}

void MainWindow::fileAppAbout()
{
    try
    {
        QMessageBox *dlg = new QMessageBox();
        dlg->setWindowTitle("About " + QCoreApplication::applicationName() + "...");
        dlg->setText(QCoreApplication::applicationName() + " version " + QCoreApplication::applicationVersion());
        dlg->exec();
    }
    catch (const std::exception& exc)
    {
        qDebug() << "EXCEPTION in fileAppAbout: " << exc.what() << endl;
    }
}

void MainWindow::fileAppNew()
{
    try
    {
        this->readLanguageList();
        //this->translate("Pull requests let you tell others about changes you've pushed to a branch");
    }
    catch (const std::exception& exc)
    {
        qDebug() << "EXCEPTION in fileAppNew: " << exc.what() << endl;
    }
}

void MainWindow::translate(const QString &value)
{
    QString jsonString = "{\"text\":[\"" + value + "\"],\"model_id\":\"en-de\"}";
    QByteArray postData = QByteArray::number(jsonString.size());

    QNetworkAccessManager * manager = new QNetworkAccessManager(this);

    QSettings settings;

    QUrlQuery params;
    params.addQueryItem("version", settings.value("IBMSettings/VersionDate").toString());

    QUrl url(settings.value("IBMSettings/ServiceUrl").toString() + "/v3/translate?" + params.query());
    QNetworkRequest request(url);

    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postData);
    request.setRawHeader("x-watson-learning-opt-out", "true");
    request.setRawHeader("Authorization", this->buildAuthorizationItem().toLocal8Bit());

    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(onRequestCompleted(QNetworkReply *)));
    manager->post(request, jsonString.toUtf8());
}

void MainWindow::onRequestCompleted(QNetworkReply *rep)
{
    try
    {
        QByteArray bts = rep->readAll();
        if(bts.length() > 0)
        {
            QString str(bts);
            QMessageBox::information(this, QCoreApplication::applicationName(),str, "ok");
            this->statusBar()->showMessage(QString("Received %1 bytes received").arg(bts.length()));
        }
        else
        {
            this->statusBar()->showMessage("Received empty response");
        }
    }
    catch (const std::exception& exc)
    {
        qDebug() << "EXCEPTION in onRequestCompleted: " << exc.what() << endl;
    }
}

QStringList MainWindow::readLanguageList()
{
    QSettings settings;

    QUrlQuery params;
    params.addQueryItem("version", settings.value("IBMSettings/VersionDate").toString());

    QUrl url(settings.value("IBMSettings/ServiceUrl").toString() + "/v3/identifiable_languages?" + params.query());
    QNetworkRequest request(url);

    request.setRawHeader("Authorization", this->buildAuthorizationItem().toLocal8Bit());

    QNetworkAccessManager * manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->get(request);

    QStringList languageList;
    connect(reply, &QNetworkReply::finished, [=]() {

        if(reply->error() == QNetworkReply::NoError)
        {
            QByteArray bts = reply->readAll();
            if(bts.length() > 0)
            {
                QString str(bts);

                QJsonDocument jsonResponse = QJsonDocument::fromJson(bts);
                QJsonObject jsonObject = jsonResponse.object();
                QJsonArray jsonArray = jsonObject["languages"].toArray();
                foreach (const QJsonValue & value, jsonArray)
                {
                    QJsonObject obj = value.toObject();
                }

                QMessageBox::information(this, QCoreApplication::applicationName(),str, "ok");
            }
        }
        else // handle error
        {
          qDebug() << reply->errorString();
        }
    });

    return languageList;
}

QString MainWindow::buildAuthorizationItem()
{
    QSettings settings;
    QString concatenated = "apikey:"+settings.value("IBMSettings/AccessToken").toString();
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    return ("Basic " + data);
}

void MainWindow::insertRow(const QString &value, const QString &translation)
{
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    int nRow = ui->tableWidget->rowCount() - 1;
    ui->tableWidget->setItem(nRow, 0, new QTableWidgetItem(QDateTime::currentDateTime().time().toString()));
    ui->tableWidget->setItem(nRow, 1, new QTableWidgetItem(value));
    ui->tableWidget->setItem(nRow, 2, new QTableWidgetItem(translation));
}
