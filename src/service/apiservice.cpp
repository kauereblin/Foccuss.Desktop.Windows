#include "apiservice.h"
#include "../data/appmodel.h"
#include "../data/blockTimeSettingsModel.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

static QString s_logFilePath;

void logToFileAS(const QString& message) {
    if (s_logFilePath.isEmpty()) {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir appDataDir(appDataPath);
        if (!appDataDir.exists()) {
            appDataDir.mkpath(".");
        }
        s_logFilePath = appDataDir.filePath("foccuss_service.log");
    }

    QFile logFile(s_logFilePath);
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") 
            << " - " << message << "\n";
        logFile.close();
    }
}

ApiService::ApiService(Database* database, QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_database(database)
{
}

ApiService::~ApiService()
{
}

void ApiService::setBaseUrl(const QString& url)
{
    m_baseUrl = url;
}

void ApiService::syncBlockedApps()
{
    if (m_baseUrl.isEmpty()) {
        emit syncFailed("Base URL not set");
        return;
    }

    QNetworkRequest request(QUrl(m_baseUrl + "/blocked-apps/windows"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonArray appsJson = blockedAppsToJson();
    QJsonDocument doc(appsJson);
    QByteArray data = doc.toJson();

    QNetworkReply* reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onBlockedAppsSyncFinished(reply);
    });
}

void ApiService::fetchBlockedApps()
{
    if (m_baseUrl.isEmpty()) {
        emit syncFailed("Base URL not set");
        return;
    }

    QNetworkRequest request(QUrl(m_baseUrl + "/blocked-apps/windows"));

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onBlockedAppsFetchFinished(reply);
    });
}

void ApiService::syncTimeSettings()
{
    if (m_baseUrl.isEmpty()) {
        emit syncFailed("Base URL not set");
        return;
    }

    QNetworkRequest request(QUrl(m_baseUrl + "/block-time-settings/windows"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject settingsJson = timeSettingsToJson();
    QJsonDocument doc(settingsJson);
    QByteArray data = doc.toJson();

    QNetworkReply* reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onTimeSettingsSyncFinished(reply);
    });
}

void ApiService::fetchTimeSettings()
{
    if (m_baseUrl.isEmpty()) {
        emit syncFailed("Base URL not set");
        return;
    }

    QNetworkRequest request(QUrl(m_baseUrl + "/block-time-settings/windows"));

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onTimeSettingsFetchFinished(reply);
    });
}

void ApiService::onBlockedAppsSyncFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        emit syncCompleted(true);
    } else {
        emit syncFailed(reply->errorString());
    }
}

void ApiService::onBlockedAppsFetchFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isArray()) {
            processBlockedAppsResponse(doc.array());
            emit dataFetched(true);
        } else {
            emit dataFetched(false);
        }
    } else {
        emit syncFailed(reply->errorString());
    }
}

void ApiService::onTimeSettingsSyncFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        emit syncCompleted(true);
    } else {
        emit syncFailed(reply->errorString());
    }
}

void ApiService::onTimeSettingsFetchFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isObject()) {
            processTimeSettingsResponse(doc.object());
            emit dataFetched(true);
        } else {
            emit dataFetched(false);
        }
    } else {
        emit syncFailed(reply->errorString());
    }
}

QJsonArray ApiService::blockedAppsToJson() const
{
    QJsonArray appsArray;
    QList<std::shared_ptr<AppModel>> blockedApps = m_database->getBlockedApps();

    for (const auto& app : blockedApps) {
        QJsonObject appObj;
        appObj["appPath"] = app->getPath();
        appObj["appName"] = app->getName();
        appObj["isBlocked"] = app->getActive();
        appsArray.append(appObj);
    }

    return appsArray;
}

QJsonObject ApiService::timeSettingsToJson() const
{
    QJsonObject settingsObj;
    auto settings = m_database->getBlockTimeSettings();

    if (settings) {
        settingsObj["startHour"] = settings->getStartTime().hour();
        settingsObj["startMinute"] = settings->getStartTime().minute();
        settingsObj["endHour"] = settings->getEndTime().hour();
        settingsObj["endMinute"] = settings->getEndTime().minute();
        
        REG_Week week = settings->getWeek();
        settingsObj["monday"] = week.monday;
        settingsObj["tuesday"] = week.tuesday;
        settingsObj["wednesday"] = week.wednesday;
        settingsObj["thursday"] = week.thursday;
        settingsObj["friday"] = week.friday;
        settingsObj["saturday"] = week.saturday;
        settingsObj["sunday"] = week.sunday;
        
        settingsObj["isActive"] = settings->getActive();
    }

    return settingsObj;
}

void ApiService::processBlockedAppsResponse(const QJsonArray& apps)
{
    QList<std::shared_ptr<AppModel>> currentApps = m_database->getBlockedApps();
    for (const auto& app : currentApps) {
        m_database->removeBlockedApp(app->getPath());
    }

    for (const QJsonValue& appValue : apps) {
        QJsonObject appObj = appValue.toObject();
        QString path = appObj["appPath"].toString();
        QString name = appObj["appName"].toString();
        if (path.isEmpty() || name.isEmpty())
            continue;

        int isBlocked = appObj["isBlocked"].toInt();
        if (isBlocked == 1)
            m_database->addBlockedApp(path, name);
        else
            m_database->removeBlockedApp(path);
    }
}

void ApiService::processTimeSettingsResponse(const QJsonObject& settings)
{
    QTime startTime(settings["startHour"].toInt(), settings["startMinute"].toInt());
    QTime endTime(settings["endHour"].toInt(), settings["endMinute"].toInt());
    
    REG_Week week = {
        settings["monday"].toBool(),
        settings["tuesday"].toBool(),
        settings["wednesday"].toBool(),
        settings["thursday"].toBool(),
        settings["friday"].toBool(),
        settings["saturday"].toBool(),
        settings["sunday"].toBool()
    };
    
    bool isActive = settings["isActive"].toBool();
    
    auto timeSettings = std::make_shared<BlockTimeSettingsModel>(startTime, endTime, week, isActive);
    m_database->updateBlockTimeSettings(timeSettings);
} 