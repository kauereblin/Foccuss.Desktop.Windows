#include "database.h"
#include "appmodel.h"
#include "blockTimeSettingsModel.h"

#include <QDir>
#include <QStandardPaths>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>
#include <QTime>
//#include <QVariant>

static QString s_logFilePath;

void _logToFile(const QString& message) {
    if (false)
    {
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
}

Database::Database() : m_initialized(false)
{
    // Set up database path in AppData location
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataLocation);
    
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    m_dbPath = dir.filePath("foccuss.db");
}

Database::~Database()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Database::initialize()
{
    QFileInfo dbFile(m_dbPath);
    if (!dbFile.exists()) {
        qDebug() << "Database file does not exist, creating new file...";
        QFile file(m_dbPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << "Failed to create database file:" << file.errorString();
            return false;
        }
        file.close();
    }
    
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_dbPath);
    
    if (!m_db.open()) {
        qDebug() << "Error opening database:" << m_db.lastError().text();
        return false;
    }
    
    if (!createTables()) {
        qDebug() << "Error creating tables";
        return false;
    }
    
    m_initialized = true;
    return true;
}

bool Database::isInitialized() const
{
    return m_initialized;
}

bool Database::createTables()
{
    QSqlQuery query(m_db);
    
    if (!query.exec("CREATE TABLE IF NOT EXISTS blocked_apps ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "app_path TEXT UNIQUE NOT NULL, "
                   "app_name TEXT NOT NULL, "
                   "is_blocked BOOLEAN NOT NULL)"))
    {
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS block_time_settings ("
                   "id INTEGER PRIMARY KEY, "
                   "start_hour INTEGER NOT NULL, "
                   "start_minute INTEGER NOT NULL, "
                   "end_hour INTEGER NOT NULL, "
                   "end_minute INTEGER NOT NULL, "
                   "monday BOOLEAN, "
                   "tuesday BOOLEAN, "
                   "wednesday BOOLEAN, "
                   "thursday BOOLEAN, "
                   "friday BOOLEAN, "
                   "saturday BOOLEAN, "
                   "sunday BOOLEAN, "
                   "is_active BOOLEAN NOT NULL)"))
    {
        return false;
    }
    
    if (!query.exec("INSERT OR IGNORE INTO block_time_settings ("
                        "id, start_hour, start_minute, end_hour, end_minute, "
                        "monday, tuesday, wednesday, thursday, friday, "
                        "saturday, sunday, is_active"
                        ") VALUES ("
                        "1, 8, 0, 17, 0, "
                        "1, 1, 1, 1, 1, "
                        "0, 0, 1)"))
    {
        return false;
    }

    return true;
}

#pragma region BlockedApp

bool Database::addBlockedApp(const QString& appPath, const QString& appName)
{
    if (!m_initialized) return false;
    
    QString normalizedPath = QDir::cleanPath(appPath).replace("\\", "/");

    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO blocked_apps (app_path, app_name, is_blocked) VALUES (:normalizedPath, :appPath, 1)");
    query.bindValue(":normalizedPath", normalizedPath);
    query.bindValue(":appPath", appName);
    
    if (!query.exec()) {
        qDebug() << "Error adding blocked app:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::removeBlockedApp(const QString& appPath)
{
    if (!m_initialized) return false;
    
    QSqlQuery query(m_db);
    query.prepare("UPDATE blocked_apps SET is_blocked = 0 WHERE app_path LIKE :path");
    query.bindValue(":path", appPath);
    
    if (!query.exec()) {
        qDebug() << "Error removing blocked app:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool Database::isAppBlocked(const QString& appPath) const
{
    if (!m_initialized) return false;

    QString normalizedPath = QDir::cleanPath(appPath).replace("\\", "/");

    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM blocked_apps WHERE app_path LIKE :path;");
    query.bindValue(":path", QVariant(normalizedPath));
    
    if (!query.exec() || !query.next())
        return false;

    return true;
}

QList<std::shared_ptr<AppModel>> Database::getBlockedApps() const
{
    QList<std::shared_ptr<AppModel>> result;
    
    if (!m_initialized) return result;
    
    QSqlQuery query(m_db);
    query.exec("SELECT app_path, app_name, is_blocked FROM blocked_apps WHERE is_blocked = 1 ORDER BY app_name");
    
    while (query.next()) {
        QString appPath = query.value(0).toString();
        QString appName = query.value(1).toString();
        
        result.append(std::make_shared<AppModel>(appPath, appName));
    }
    
    return result;
}

#pragma endregion BlockedApp

#pragma region BlockTimeSettings

std::shared_ptr<BlockTimeSettingsModel> Database::getBlockTimeSettings() const
{
    if (!m_initialized) return nullptr;
    
    QSqlQuery query(m_db);
    if (!query.exec("SELECT start_hour, start_minute, end_hour, end_minute, "
                    "monday, tuesday, wednesday, thursday, friday, saturday, sunday, is_active "
                    "FROM block_time_settings WHERE id = 1")) {
        _logToFile("getBlockTimeSettings failed: " + query.lastError().text());
        return nullptr;
    }
    
    if (query.next()) {
        int startHour = query.value(0).toInt();
        int startMinute = query.value(1).toInt();
        int endHour = query.value(2).toInt();
        int endMinute = query.value(3).toInt();
        
        REG_Week week;
        week.monday = query.value(4).toBool();
        week.tuesday = query.value(5).toBool();
        week.wednesday = query.value(6).toBool();
        week.thursday = query.value(7).toBool();
        week.friday = query.value(8).toBool();
        week.saturday = query.value(9).toBool();
        week.sunday = query.value(10).toBool();
        
        bool isActive = query.value(11).toBool();
        
        QTime startTime(startHour, startMinute);
        QTime endTime(endHour, endMinute);
        
        return std::make_shared<BlockTimeSettingsModel>(startTime, endTime, week, isActive);
    }

    return nullptr;
}

bool Database::updateBlockTimeSettings(const std::shared_ptr<BlockTimeSettingsModel>& settings)
{
    if (!m_initialized || !settings) return false;
    
    QTime startTime = settings->getStartTime();
    QTime endTime = settings->getEndTime();
    REG_Week week = settings->getWeek();
    bool isActive = settings->getActive();
    
    QSqlQuery query(m_db);
    query.prepare("UPDATE block_time_settings SET "
                 "start_hour = :startHour, "
                 "start_minute = :startMinute, "
                 "end_hour = :endHour, "
                 "end_minute = :endMinute, "
                 "monday = :monday, "
                 "tuesday = :tuesday, "
                 "wednesday = :wednesday, "
                 "thursday = :thursday, "
                 "friday = :friday, "
                 "saturday = :saturday, "
                 "sunday = :sunday, "
                 "is_active = :isActive "
                 "WHERE id = 1");
    
    query.bindValue(":startHour", startTime.hour());
    query.bindValue(":startMinute", startTime.minute());
    query.bindValue(":endHour", endTime.hour());
    query.bindValue(":endMinute", endTime.minute());
    query.bindValue(":monday", week.monday);
    query.bindValue(":tuesday", week.tuesday);
    query.bindValue(":wednesday", week.wednesday);
    query.bindValue(":thursday", week.thursday);
    query.bindValue(":friday", week.friday);
    query.bindValue(":saturday", week.saturday);
    query.bindValue(":sunday", week.sunday);
    query.bindValue(":isActive", isActive);
    
    if (!query.exec()) {
        _logToFile("updateBlockTimeSettings failed: " + query.lastError().text());
        return false;
    }
    
    return true;
}

bool Database::isBlockingActive() const
{
    if (!m_initialized) return false;

    QSqlQuery query(m_db);
    if (!query.exec("SELECT is_active FROM block_time_settings WHERE id = 1")) {
        _logToFile("isBlockingActive failed: " + query.lastError().text());
        return false;
    }
    
    if (query.next())
        return query.value(0).toBool();

    return false;
}

bool Database::isBlockingNow() const
{
    if (!m_initialized) return false;

    auto settings = getBlockTimeSettings();
    if (!settings || !settings->getActive()) {
        return false;
    }

    QTime currentTime = QTime::currentTime();
    QTime startTime = settings->getStartTime();
    QTime endTime = settings->getEndTime();

    // Check if current time is within blocking hours
    bool isWithinTimeRange = false;
    if (startTime < endTime)
        isWithinTimeRange = (currentTime >= startTime && currentTime <= endTime);
    else
        isWithinTimeRange = (currentTime >= startTime || currentTime <= endTime);
    
    if (!isWithinTimeRange)
        return false;

    // Check if current day is enabled
    QDate currentDate = QDate::currentDate();
    int currentDay = currentDate.dayOfWeek();
    REG_Week week = settings->getWeek();
    
    switch (currentDay) {
        case 1: return week.monday;
        case 2: return week.tuesday;
        case 3: return week.wednesday;
        case 4: return week.thursday;
        case 5: return week.friday;
        case 6: return week.saturday;
        case 7: return week.sunday;
        default: return false;
    }
}

#pragma endregion BlockTimeSettings