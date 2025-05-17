#ifndef APISERVICE_H
#define APISERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>
#include "../data/database.h"

class ApiService : public QObject
{
    Q_OBJECT

public:
    explicit ApiService(Database* database, QObject *parent = nullptr);
    ~ApiService();

    void setBaseUrl(const QString& url);

    void syncBlockedApps();
    void fetchBlockedApps();

    void syncTimeSettings();
    void fetchTimeSettings();

signals:
    void syncCompleted(bool success);
    void syncFailed(const QString& error);
    void dataFetched(bool success);

private slots:
    void onBlockedAppsSyncFinished(QNetworkReply* reply);
    void onBlockedAppsFetchFinished(QNetworkReply* reply);
    void onTimeSettingsSyncFinished(QNetworkReply* reply);
    void onTimeSettingsFetchFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkManager;
    Database* m_database;
    QString m_baseUrl;

    QJsonArray blockedAppsToJson() const;
    QJsonObject timeSettingsToJson() const;
    void processBlockedAppsResponse(const QJsonArray& apps);
    void processTimeSettingsResponse(const QJsonObject& settings);
};

#endif // APISERVICE_H 