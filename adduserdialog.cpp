#include "adduserdialog.h"
#include "ui_adduserdialog.h"

QT_USE_NAMESPACE

static const char blankString[] = QT_TRANSLATE_NOOP("AddUserDialog", "N/A");

AddUserDialog::AddUserDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddUserDialog)
{
    ui->setupUi(this);
}

AddUserDialog::AddUserDialog(unsigned uid) : AddUserDialog() {
    ui->lineEditId->setText(QString::number(uid));
}

AddUserDialog::~AddUserDialog()
{
    delete ui;
}

void AddUserDialog::on_lineEditName_textChanged(const QString &arg1)
{
    ui_user_info.name = arg1;
}

void AddUserDialog::on_lineEditSurname_textChanged(const QString &arg1)
{
    ui_user_info.surname = arg1;
}

void AddUserDialog::on_submitButton_clicked()
{
    db = QSqlDatabase::addDatabase("QMYSQL", "AddUserDialogConnecion");
    db.setPort(3306);
    db.setDatabaseName("rfid_db");
    db.setUserName("root");
    db.setPassword("");

    if(!db.open()){
        qDebug("Failed to connect do the database");
        return;
    }

    QSqlQuery q(db);
    q.prepare("INSERT INTO users VALUES (?, ?, ?)");
    q.addBindValue(QNUM(ui_user_info.id));
    q.addBindValue(ui_user_info.name);
    q.addBindValue(ui_user_info.surname);

    if(!q.exec()){
        qDebug("Query failure: %s", CSTR(q.executedQuery()));
    }
    qDebug("DB query: %s\n", CSTR(q.executedQuery()));

    db.close();
}
