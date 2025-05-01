#ifndef APPMONITOR_H
#define APPMONITOR_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QHash>
#include <QString>
#include <QSet>
#include <memory>
#include <Windows.h>

class AppModel;
class Database;

class AppMonitor : public QObject
{
    Q_OBJECT

public:
    explicit AppMonitor(Database* database, QObject *parent = nullptr);
    
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
signals:
    void blockedAppLaunched(const HWND targetWindow, const QString& appPath, const QString& appName);
    
private slots:
    void checkRunningApps();
    
private:
    Database* m_database;
    QTimer m_monitorTimer;
    bool m_isMonitoring;
    
    // Cache previously detected processes to avoid repeatedly signaling
    QSet<HWND> m_HwndCache;
};

#endif // APPMONITOR_H 