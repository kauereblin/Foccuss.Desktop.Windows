#ifndef APPDETECTOR_H
#define APPDETECTOR_H

#include <QObject>
#include <QList>
#include <QSettings>
#include <memory>

class AppModel;

class AppDetector : public QObject
{
    Q_OBJECT

public:
    explicit AppDetector(QObject *parent = nullptr);
    QList<std::shared_ptr<AppModel>> getInstalledApps() const;
    QList<std::shared_ptr<AppModel>> getRunningApps() const;
    void refreshInstalledApps();
    
signals:
    void installedAppsChanged();
    
private:
    void findAppsInRegistry();
    void processRegistryApps(QSettings& registry);
    
    QString findExecutableInDirectory(const QString& directory, const QString& appName);
    QString extractExecutableFromDisplayIcon(const QString& displayIcon);
    QString extractExecutableFromAppKey(const QString& appKey, const QString& displayName);
    QString inferExecutableFromUninstallString(const QString& uninstallString, const QString& displayName);
    QString findCommonExecutablePath(const QString& appName, const QString& appKey);

    QList<std::shared_ptr<AppModel>> m_installedApps;
};

#endif // APPDETECTOR_H 