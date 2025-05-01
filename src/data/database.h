#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QSqlDatabase>
#include <QList>
#include <memory>

class AppModel;
class BlockTimeSettingsModel;
struct REG_Week;

class Database
{
public:
    Database();
    ~Database();

    bool initialize();
    bool isInitialized() const;
    
    bool addBlockedApp(const QString& appPath, const QString& appName);
    bool removeBlockedApp(const QString& appPath);
    bool isAppBlocked(const QString& appPath) const;
    QList<std::shared_ptr<AppModel>> getBlockedApps() const;

    std::shared_ptr<BlockTimeSettingsModel> getBlockTimeSettings() const;
    bool updateBlockTimeSettings(const std::shared_ptr<BlockTimeSettingsModel>& settings);
    bool isBlockingActive() const;
    bool isBlockingNow() const;

private:
    bool createTables();
    
    QSqlDatabase m_db;
    bool m_initialized;
    QString m_dbPath;
};

#endif // DATABASE_H 