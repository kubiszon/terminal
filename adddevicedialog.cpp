#include "adddevicedialog.h"
#include "ui_adddevicedialog.h"

QT_USE_NAMESPACE

static const char blankString[] = QT_TRANSLATE_NOOP("AddDeviceDialog", "N/A");

AddDeviceDialog::AddDeviceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDeviceDialog)
{
    ui->setupUi(this);
}

AddDeviceDialog::AddDeviceDialog(unsigned did) : AddDeviceDialog() {
    ui->lineEditId->setText(QString::number(did));
}

AddDeviceDialog::~AddDeviceDialog()
{
    delete ui;
}

void AddDeviceDialog::on_lineEditName_textChanged(const QString &arg1)
{
    ui_dev_info.name = arg1;
}

void AddDeviceDialog::on_lineEditModel_textChanged(const QString &arg1)
{
    ui_dev_info.model = arg1;
}

void AddDeviceDialog::on_lineEditYear_textChanged(const QString &arg1)
{
    ui_dev_info.year = arg1.toUInt();
}

void AddDeviceDialog::on_submitButton_clicked()
{
    db = QSqlDatabase::addDatabase("QMYSQL", "AddDeviceDialogConnecion");
    db.setPort(3306);
    db.setDatabaseName("rfid_db");
    db.setUserName("root");
    db.setPassword("");

    if(!db.open()){
        qDebug("Failed to connect do the database");
        return;
    }

    QSqlQuery q(db);
    q.prepare("INSERT INTO devices VALUES (?, ?, ?, ?, ?)");
    q.addBindValue(QNUM(ui_dev_info.id));
    q.addBindValue("0");
    q.addBindValue(ui_dev_info.model);
    q.addBindValue(ui_dev_info.name);
    q.addBindValue(ui_dev_info.year);

    if(!q.exec()){
        qDebug("Query failure: %s", CSTR(q.executedQuery()));
    }
    qDebug("DB query: %s\n", CSTR(q.executedQuery()));

    db.close();
}
