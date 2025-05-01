#ifndef WINSERVICE_H
#define WINSERVICE_H

#include <QObject>
#include <QString>
#include <Windows.h>
#include <QCoreApplication>

class AppMonitor;
class Database;

class WinService : public QObject
{
    Q_OBJECT
    
public:
    explicit WinService(Database* database, QObject *parent = nullptr);
    ~WinService();
    
    bool initialize();
    bool installService();
    bool uninstallService();
    bool startService();
    bool stopService();
    bool isServiceInstalled() const;
    bool isServiceRunning() const;
    
    QString getServiceName() const;
    QString getServiceDisplayName() const;
    
    static VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
    
private:
    static VOID WINAPI ServiceCtrlHandler(DWORD control);
    static WinService* s_instance;
    static std::unique_ptr<QCoreApplication> s_appInstance;
    
    static void reportServiceStatus(DWORD currentState, DWORD exitCode = NO_ERROR, DWORD waitHint = 0);
    void serviceWorkerThread();
    
    // Service name and status
    QString m_serviceName;
    QString m_serviceDisplayName;
    static SERVICE_STATUS m_serviceStatus;
    static SERVICE_STATUS_HANDLE m_serviceStatusHandle;
    
    // Service components
    Database* m_database;
    AppMonitor* m_appMonitor;
    
    // Control event
    HANDLE m_serviceStopEvent;
};

#endif // WINSERVICE_H 