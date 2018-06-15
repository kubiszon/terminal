#ifndef USER_H
#define USER_H

typedef struct TransactionInfo TransactionInfo;
struct TransactionInfo {
    unsigned id;
    unsigned user_id;
    unsigned device_id;
    QDate rental_date;
    QDate return_date;
    bool archived;
};

typedef struct UserInfo UserInfo;
struct UserInfo {
    unsigned id;
    QString name;
    QString surname;
    unsigned borrowed_devs;
    QList<TransactionInfo> transactions;
};

typedef struct DeviceInfo DeviceInfo;
struct DeviceInfo {
    unsigned id;
    unsigned user_id;
    QString model;
    QString name;
    unsigned year;
};

#endif // USER_H
