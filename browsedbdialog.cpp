#include "browsedbdialog.h"

QT_USE_NAMESPACE

static const char blankString[] = QT_TRANSLATE_NOOP("BrowseDbDialog", "N/A");

BrowseDbDialog::BrowseDbDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BrowseDbDialog)
{
    ui->setupUi(this);

    db = QSqlDatabase::addDatabase("QMYSQL", "BrowseDialogConnecion");
    db.setPort(3306);
    db.setDatabaseName("rfid_db");
    db.setUserName("root");
    db.setPassword("");
    FillTables();
}

void BrowseDbDialog::FillTables()
{
    if(!db.open()){
        qDebug("Failed to connect do the database");
        return;
    }

    QSqlTableModel *usersModel = new QSqlTableModel(ui->usersTable, db);
    QSqlTableModel *devicesModel = new QSqlTableModel(ui->devicesTable, db);
    QSqlTableModel *transactionsModel = new QSqlTableModel(ui->transactionsTable, db);

    usersModel->setTable("users");
    usersModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    usersModel->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    usersModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Imię"));
    usersModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Nazwisko"));
    usersModel->select();

    devicesModel->setTable("devices");
    devicesModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    devicesModel->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    devicesModel->setHeaderData(1, Qt::Horizontal, QObject::tr("ID użytkownika"));
    devicesModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Model"));
    devicesModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Nazwa"));
    devicesModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Rok produkcji"));
    devicesModel->select();

    transactionsModel->setTable("transactions");
    transactionsModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    transactionsModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Numer transakcji"));
    transactionsModel->setHeaderData(1, Qt::Horizontal, QObject::tr("ID użytkownika"));
    transactionsModel->setHeaderData(2, Qt::Horizontal, QObject::tr("ID narzędzia"));
    transactionsModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Data wypożyczenia"));
    transactionsModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Data oddania"));
    transactionsModel->setHeaderData(5, Qt::Horizontal, QObject::tr("Archiwalne"));
    transactionsModel->select();
    db.close();

    ui->usersTable->setModel(usersModel);
    ui->usersTable->resizeColumnsToContents();
    ui->devicesTable->setModel(devicesModel);
    ui->devicesTable->resizeColumnsToContents();
    ui->transactionsTable->setModel(transactionsModel);
    ui->transactionsTable->resizeColumnsToContents();
}

BrowseDbDialog::~BrowseDbDialog()
{
    delete ui;
}


void BrowseDbDialog::on_refreshButton_clicked()
{
    FillTables();
    this->repaint();
}
