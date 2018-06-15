/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"

QString msg            = "_______________________________";
QString app_ready_msg  = "APPLICATION INITIALIZED________";
QString app_reset_msg  = "APPLICATION RESTARTED__________";
QString no_user_msg    = "USER NOT FOUND IN DATABASE_____";
QString user_added_msg = "USER ADDED TO DATABASE_________";
QString dev_borrow_msg = "DEVICE HAS BEEN BORROWED_______";
QString dev_return_msg = "DEVICE HAS BEEN RETURNED_______";
QString no_dev_msg     = "DEVICE DOES NOT EXIST__________";
QString userIdPrefix   = "UID:";

void infoMessage(QMainWindow *obj, QString msg){
    QMessageBox::information(obj, obj->tr("Informacja"), obj->tr(CSTR(msg)));
}

void warningMessage(QMainWindow *obj, QString msg) {
    QMessageBox::warning(obj, obj->tr("Ostrzeżenie"), obj->tr(CSTR(msg)));
}

void criticalMessage(QMainWindow *obj, QString msg){
    QMessageBox::critical(obj, obj->tr("Błąd krytyczny"), obj->tr(CSTR(msg)));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    console = new Console;
    console->setEnabled(false);
    setCentralWidget(console);
    serial = new QSerialPort(this);
    settings = new SettingsDialog;
    tables = new BrowseDbDialog;

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionConfigure->setEnabled(true);
    status = new QLabel;
    ui->statusBar->addWidget(status);

    initActionsConnections();
    connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &MainWindow::handleError);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(console, &Console::getData, this, &MainWindow::writeData);

    initMySQLConnection();
    ui->actionConnect->trigger();
}

MainWindow::~MainWindow()
{
    delete tables;
    delete settings;
    delete ui;
}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settings->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {
        console->setEnabled(true);
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());

        showStatusMessage(tr("Open error"));
    }
    QThread::usleep(200000);
    serial->write(QByteArray(CSTR(app_reset_msg)));
    QThread::usleep(200000);
    serial->write(QByteArray(CSTR(app_ready_msg)));

    console->log("Aplikacja gotowa");
    console->log("\nOczekiwanie na użytkownika...");
}

void MainWindow::restartApp()
{
    QThread::usleep(200000);
    serial->write(QByteArray(CSTR(app_reset_msg)));
    currentUser = 0;
    currentDevice = 0;
    userValid = false;
    QThread::usleep(200000);
    serial->write(QByteArray(CSTR(app_ready_msg)));
    console->log("\nOczekiwanie na użytkownika...");
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
    console->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("O programie"),
                       tr("Program <b>Rejestrator wypożyczeń narzędzi</b> ma na celu "
                          "ułatwienie ewidencji urządzeń i użytkowników "
                          "poprzez rejestrowanie transakcji w bazie danych"));
}

void MainWindow::writeData(const QByteArray &data)
{
    serial->write(data);
}

void MainWindow::readData()
{
    static QByteArray byteArray;
    byteArray += serial->readAll();

    if(!QString(byteArray).contains("\n"))
       return;

    QString data = QString( byteArray ).remove("\r").remove("\n");
    byteArray.clear();

    qDebug("data: %s", CSTR(data));

    if (!userValid){
        console->log("\n___________________________________________________________");
        clearUserAndDevInfo();
        QString userStr = data.mid(data.indexOf(userIdPrefix) + userIdPrefix.length());
        currentUser = userStr.toUInt();
        if(getUserInfo(currentUser)) {
            userValid = true;
            QString user_name_msg = msg;
            QString msg_text = user_info.name + "." + user_info.surname;
            console->log("Użytkownik: %s %s, ID %d", CSTR(user_info.name), CSTR(user_info.surname), user_info.id);
            for (int i = 0; i < (msg_text.length()) && i < 31 ; i++)
                user_name_msg[i] = msg_text[i];
            serial->write(QByteArray(CSTR(user_name_msg)));
            getUserTransactionInfo(currentUser);
        } else {
            unsigned temp_id = currentUser;
            serial->write(QByteArray(CSTR(no_user_msg)));
            //restartApp();
            console->log("Brak użytkownika o ID=%d w bazie", temp_id);
            askForDataInsertion("Brak użytkownika w bazie. Dodać użytkownika?", USER, temp_id);
            restartApp();
            return;
        }
        serial->readAll();
        return;
    }
    if (userValid){
        QString deviceStr = data.mid(data.indexOf(":") + 1);
        currentDevice = deviceStr.toUInt();
        qDebug("ID narzędzia: %d", currentDevice);
        if (currentDevice == currentUser) {
            warningMessage(this, "ID narzędzia jest równe ID użytkownika");
            restartApp();
            return;
        }
        if(getDeviceInfo(currentDevice)) {
            console->log("ID narzędzia: %d", dev_info.id);
            console->log("%s, %s, %d, %s", CSTR(dev_info.name), CSTR(dev_info.model),
                    dev_info.year, dev_info.user_id ? "wypożyczone" : "dostępne");
            if(isDeviceAssignedToUser(currentUser, currentDevice)) {
                deassignDeviceFromUser(currentUser, currentDevice);
                console->log("Użytkownik %d zwrócił narzędzie %d", currentUser, currentDevice);
                serial->write(QByteArray(CSTR(dev_return_msg)));
            } else {
                assignDeviceToUser(currentUser, currentDevice);
                console->log("Użytkownik %d wypożyczył narzędzie %d", currentUser, currentDevice);
                serial->write(QByteArray(CSTR(dev_borrow_msg)));
            }
        }
        else {
            console->log("", currentDevice);
            serial->write(QByteArray(CSTR(no_dev_msg)));
            askForDataInsertion("Brak urządzenia w bazie. Dodać narzędzie?",
                                DEVICE, currentDevice);
        }
        restartApp();
        tables->FillTables();
        tables->repaint();
    }
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionConfigure, &QAction::triggered, settings, &MainWindow::show);
    connect(ui->actionClear, &QAction::triggered, console, &Console::clear);
    connect(ui->actionRestartApp, &QAction::triggered, this, &MainWindow::restartApp);
    connect(ui->actionSql, &QAction::triggered, tables, &MainWindow::show);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
}

void MainWindow::initMySQLConnection(void)
{
    db = QSqlDatabase::addDatabase("QMYSQL", "BoardDatabaseConnection");
    db.setHostName("localhost");
    db.setPort(3306);
    db.setDatabaseName("rfid_db");
    db.setUserName("root");
    db.setPassword("");
}

bool MainWindow::getUserInfo(unsigned uid){
    if(!db.open()){
        criticalMessage(this, "Nie udało się połączyć z bazą danych");
        return false;
    }
    else {
        QSqlQuery sqlQuery(db);
        QString query = "SELECT * FROM users WHERE user_id=" + QString::number(uid);
        qDebug("DB query: %s", CSTR(query));
        if(!sqlQuery.exec(query) || !sqlQuery.first()){
            db.close();
            return false;
        }
        else {
            user_info.id      = sqlQuery.value("user_id").toUInt();
            user_info.name    = sqlQuery.value("name").toString();
            user_info.surname = sqlQuery.value("surname").toString();
            qDebug("user: %d, name: %s, surname: %s", user_info.id, CSTR(user_info.name), CSTR(user_info.surname));
        }
    }
    db.close();
    return true;
}

bool MainWindow::getUserTransactionInfo(unsigned uid){
    if(!db.open()){
        criticalMessage(this, "Nie udało się połączyć z bazą danych");
        return false;
    }
    else {
        QSqlQuery sqlQuery(db);
        QString query = "SELECT * FROM transactions WHERE user_id=" + QString::number(uid);
        qDebug("DB query: %s",  CSTR(query));
        if(!sqlQuery.exec(query) || !sqlQuery.first()){
            console->log("Brak transakcji dla tego użytkownika");
            db.close();
            return true;
        }

        do {
            TransactionInfo tran_info;
            tran_info.id          = sqlQuery.value("transaction_id").toUInt();
            tran_info.user_id     = sqlQuery.value("user_id").toUInt();
            tran_info.device_id   = sqlQuery.value("device_id").toUInt();
            tran_info.rental_date = sqlQuery.value("rental_date").toDate();
            tran_info.return_date = sqlQuery.value("return_date").toDate();
            tran_info.archived    = sqlQuery.value("archived").toBool();
            if (!tran_info.archived) {
                user_info.transactions.append(tran_info);
                qDebug("transaction: %d, user: %d, device: %d",
                       tran_info.id, tran_info.user_id, tran_info.device_id);
            }
        } while (sqlQuery.next());
        console->log("Użytkownik %d posiada %d wypożyczonych narzędzi", uid, user_info.transactions.length());
    }

    db.close();
    return true;
}

bool MainWindow::getDeviceInfo(unsigned did){
    if(!db.open()){
        criticalMessage(this, "Nie udało się połączyć z bazą danych");
        return false;
    }
    else
    {
        QSqlQuery sqlQuery(db);
        QString query = "SELECT * FROM devices WHERE device_id=" + QString::number(did);
        qDebug("DB query: %s", CSTR(query));
        if(!sqlQuery.exec(query)) {
            criticalMessage(this, "Zapytanie nie powiodło się");
            db.close();
            return false;
        }
        if (!sqlQuery.first()){
            console->log("Brak urządzenia %d w bazie", did);
            db.close();
            return false;
        }
        else {
            dev_info.id      = sqlQuery.value("device_id").toUInt();
            dev_info.user_id = sqlQuery.value("user_id").toUInt();
            dev_info.name    = sqlQuery.value("device_name").toString();
            dev_info.model   = sqlQuery.value("device_model").toString();
            dev_info.year    = sqlQuery.value("device_year").toUInt();
            qDebug("id: %d, user_id: %d, name: %s, model: %s, year: %d",
                    dev_info.id, dev_info.user_id, CSTR(dev_info.name), CSTR(dev_info.model), dev_info.year);
        }
    }

    db.close();
    return true;
}

bool MainWindow::isDeviceAssignedToUser(unsigned uid, unsigned did) {
    for (auto t : user_info.transactions){
        qDebug("User %d device: %d", t.user_id, t.device_id);
        if (did == t.device_id && uid == t.user_id && !t.archived){
            console->log("Znaleziono narzędzie %d przypisane do użytkownika %d", did, uid);
            return true;
        }
    }
    return false;
}

bool MainWindow::assignDeviceToUser(unsigned uid, unsigned did){
    if(!db.open()){
        criticalMessage(this, "Nie udało się połączyć z bazą danych");
        return false;
    }
    QSqlQuery q(db), q2(db);
    q.prepare("INSERT INTO transactions VALUES (NULL, ?, ?, ?, ?, ?)");
    q.addBindValue(QNUM(uid));
    q.addBindValue(QNUM(did));
    q.addBindValue(QDate::currentDate());
    q.addBindValue(0);
    q.addBindValue(false);
    if (!simpleDbQuery(q)) {
        db.close();
        return false;
    }

    q2.prepare("UPDATE devices SET user_id=? WHERE device_id=?");
    q2.addBindValue(QNUM(uid));
    q2.addBindValue(QNUM(did));
    if (!simpleDbQuery(q2)) {
        db.close();
        return false;
    }

    db.close();
    return true;
}

bool MainWindow::deassignDeviceFromUser(unsigned uid, unsigned did){
    if(!db.open()){
        criticalMessage(this, "Nie udało się połączyć z bazą danych");
        return false;
    }
    QSqlQuery q(db), q2(db);
    q.prepare("UPDATE transactions SET return_date=?, archived=True WHERE device_id=? AND user_id=?");
    q.addBindValue(QDate::currentDate());
    q.addBindValue(QNUM(did));
    q.addBindValue(QNUM(uid));
    if (!simpleDbQuery(q)) {
        db.close();
        return false;
    }

    q2.prepare("UPDATE devices SET user_id=0 WHERE device_id=? AND user_id=?");
    q2.addBindValue(QNUM(did));
    q2.addBindValue(QNUM(uid));
    if (!simpleDbQuery(q2)) {
        db.close();
        return false;
    }

    db.close();
    return true;
}

bool MainWindow::simpleDbQuery(QSqlQuery sqlQuery){
    if(!sqlQuery.exec()){
        warningMessage(this, "Zapytanie nie powiodło się");
        qDebug("Query failure: %s", CSTR(sqlQuery.executedQuery()));
        return false;
    }
    QSqlRecord rec = sqlQuery.record();
    qDebug("DB query: %s", CSTR(sqlQuery.executedQuery()));
    for (int i=0; i<rec.count(); i++)
        qDebug("%s ", rec.value(i).toString());
    return true;
}

void MainWindow::openUserAddDialog(unsigned uid){
    useradd = new AddUserDialog(uid);
    useradd->ui_user_info.id = uid;
    useradd->show();
}

void MainWindow::openDeviceAddDialog(unsigned did){
    deviceadd = new AddDeviceDialog(did);
    deviceadd->ui_dev_info.id = did;
    deviceadd->show();
}

void MainWindow::askForDataInsertion(QString msg, itemToAdd item, unsigned id) {
    switch( QMessageBox::question(this, tr("Wymagana akcja"), tr(CSTR(msg)),
                QMessageBox::Yes | QMessageBox::No))
    {
      case QMessageBox::Yes:
        if (item == USER)
            openUserAddDialog(id);
        else if (item == DEVICE)
            openDeviceAddDialog(id);
        break;
      case QMessageBox::No:
        break;
      default:
        break;
    }
}

void MainWindow::clearUserAndDevInfo() {
    user_info.transactions.clear();
    user_info.id = 0;
    user_info.name = "";
    user_info.surname = "";
    user_info.borrowed_devs = 0;
    dev_info.id = 0;
    dev_info.name = "";
    dev_info.model = "";
    dev_info.year = 0;
    dev_info.user_id = 0;
}
