#include "winservice.h"
#include "../core/appmonitor.h"
#include "../data/database.h"

#include <comdef.h>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <thread>
#include <QMessageBox>
#include <windows.h>
#include <winerror.h>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QMetaObject>

// Initialize static instance pointer
WinService* WinService::s_instance = nullptr;
SERVICE_STATUS WinService::m_serviceStatus;
SERVICE_STATUS_HANDLE WinService::m_serviceStatusHandle;
std::unique_ptr<QCoreApplication> WinService::s_appInstance = nullptr;

static QString s_logFilePath;

void logToFile(const QString& message) {
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

WinService::WinService(Database* database, QObject *parent)
    : QObject(parent),
      m_serviceName("FoccussService"),
      m_serviceDisplayName("Foccuss Service"),
      m_database(database),
      m_appMonitor(nullptr),
      m_serviceStopEvent(nullptr)
{
    s_instance = this;
    
    // Initialize service status
    m_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    m_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    m_serviceStatus.dwWin32ExitCode = NO_ERROR;
    m_serviceStatus.dwServiceSpecificExitCode = 0;
    m_serviceStatus.dwCheckPoint = 0;
    m_serviceStatus.dwWaitHint = 3000;
}

WinService::~WinService()
{
    if (m_appMonitor) {
        m_appMonitor->stopMonitoring();
        delete m_appMonitor;
    }
    
    if (m_serviceStopEvent) {
        CloseHandle(m_serviceStopEvent);
    }
    
    s_instance = nullptr;
}

bool WinService::initialize()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static const char* argv[] = {m_serviceName.toStdString().c_str()};
        s_appInstance = std::make_unique<QCoreApplication>(argc, const_cast<char**>(argv));
    }

    if (!m_database || !m_database->isInitialized()) {
        logToFile("Database not initialized");
        return false;
    }
    
    m_appMonitor = new AppMonitor(m_database, this);
    if (!m_appMonitor) {
        logToFile("Failed to create AppMonitor");
        return false;
    }
    
    logToFile("AppMonitor created successfully");
    
    m_serviceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_serviceStopEvent == NULL) {
        logToFile("Failed to create service stop event");
        return false;
    }
    
    return true;
}

bool WinService::installService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL) {
        DWORD error = GetLastError();
        logToFile("Failed to open Service Control Manager. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
        return false;
    }
    
    QString appPath = QCoreApplication::applicationFilePath();
    appPath = QDir::toNativeSeparators(appPath);
    appPath += " --service";
    
    SC_HANDLE schService = CreateService(
        schSCManager,
        (const wchar_t*)m_serviceName.utf16(),
        (const wchar_t*)m_serviceDisplayName.utf16(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        (const wchar_t*)appPath.utf16(),
        NULL, NULL, NULL, NULL, NULL);
    
    if (schService == NULL) {
        DWORD error = GetLastError();
        logToFile("Failed to create service. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
        CloseServiceHandle(schSCManager);
        return false;
    }
    logToFile("Successfully created service");

    // Set service description
    SERVICE_DESCRIPTION sd;
    sd.lpDescription = (LPWSTR)L"Monitors and blocks specified applications from running";
    if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd)) {
        DWORD error = GetLastError();
        logToFile("Warning: Failed to set service description. Error code: " + QString::number(error));
    }

    // Set service to run with LocalSystem account (highest privileges)
    SERVICE_DELAYED_AUTO_START_INFO delayedStart;
    delayedStart.fDelayedAutostart = TRUE;
    if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayedStart)) {
        DWORD error = GetLastError();
        logToFile("Warning: Failed to set delayed auto-start. Error code: " + QString::number(error));
    }

    // Set service to restart on failure
    SERVICE_FAILURE_ACTIONS failureActions;
    SC_ACTION actions[3];
    actions[0].Type = SC_ACTION_RESTART;
    actions[0].Delay = 60000;  // 1 minute
    actions[1].Type = SC_ACTION_RESTART;
    actions[1].Delay = 60000;  // 1 minute
    actions[2].Type = SC_ACTION_RESTART;
    actions[2].Delay = 60000;  // 1 minute
    failureActions.cActions = 3;
    failureActions.lpsaActions = actions;
    if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions)) {
        DWORD error = GetLastError();
        logToFile("Warning: Failed to set failure actions. Error code: " + QString::number(error));
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    
    return true;
}

bool WinService::uninstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        DWORD error = GetLastError();
        logToFile("Failed to open Service Control Manager. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
        return false;
    }
    
    SC_HANDLE schService = OpenService(
        schSCManager,
        (const wchar_t*)m_serviceName.utf16(),
        DELETE);
    
    if (schService == NULL) {
        DWORD error = GetLastError();
        logToFile("Failed to open service. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
        CloseServiceHandle(schSCManager);
        return false;
    }
    
    BOOL success = DeleteService(schService);
    if (!success) {
        DWORD error = GetLastError();
        logToFile("Failed to delete service. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
    }
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    
    return success ? true : false;
}

bool WinService::startService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        DWORD error = GetLastError();
        logToFile("Failed to open Service Control Manager. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
        return false;
    }
    
    SC_HANDLE schService = OpenService(
        schSCManager,
        (const wchar_t*)m_serviceName.utf16(),
        SERVICE_START);
    
    if (schService == NULL) {
        DWORD error = GetLastError();
        logToFile("Failed to open service. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
        CloseServiceHandle(schSCManager);
        return false;
    }
    
    BOOL success = ::StartService(schService, 0, NULL);
    if (!success) {
        DWORD error = GetLastError();
        logToFile("Failed to start service. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
    }
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    
    return success ? true : false;
}

bool WinService::stopService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        DWORD error = GetLastError();
        logToFile("Failed to open Service Control Manager. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
        return false;
    }
    
    SC_HANDLE schService = OpenService(
        schSCManager,
        (const wchar_t*)m_serviceName.utf16(),
        SERVICE_STOP);
    
    if (schService == NULL) {
        DWORD error = GetLastError();
        logToFile("Failed to open service. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
        CloseServiceHandle(schSCManager);
        return false;
    }
    
    SERVICE_STATUS status;
    BOOL success = ControlService(schService, SERVICE_CONTROL_STOP, &status);
    if (!success) {
        DWORD error = GetLastError();
        logToFile("Failed to stop service. Error code: " + QString::number(error));
        logToFile("Error message: " + QString::fromWCharArray(_com_error(error).ErrorMessage()));
    }
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    
    return success ? true : false;
}

bool WinService::isServiceInstalled() const
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        return false;
    }
    
    SC_HANDLE schService = OpenService(
        schSCManager,
        (const wchar_t*)m_serviceName.utf16(),
        SERVICE_QUERY_STATUS);
    
    if (schService == NULL) {
        CloseServiceHandle(schSCManager);
        return false;
    }
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    
    return true;
}

bool WinService::isServiceRunning() const
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        return false;
    }
    
    SC_HANDLE schService = OpenService(
        schSCManager,
        (const wchar_t*)m_serviceName.utf16(),
        SERVICE_QUERY_STATUS);
    
    if (schService == NULL) {
        CloseServiceHandle(schSCManager);
        return false;
    }
    
    SERVICE_STATUS status;
    BOOL success = QueryServiceStatus(schService, &status);
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    
    if (success == FALSE) {
        return false;
    }
    
    return status.dwCurrentState == SERVICE_RUNNING;
}

QString WinService::getServiceName() const
{
    return m_serviceName;
}

QString WinService::getServiceDisplayName() const
{
    return m_serviceDisplayName;
}

VOID WINAPI WinService::ServiceMain(DWORD argc, LPWSTR* argv)
{
    if (!s_instance)
        return;

    s_instance->m_serviceStatusHandle = RegisterServiceCtrlHandler(
        (const wchar_t*)s_instance->m_serviceName.utf16(),
        ServiceCtrlHandler);
    
    if (s_instance->m_serviceStatusHandle == NULL) {
        logToFile(QString("m_serviceStatusHandle == NULL"));
        return;
    }
    
    s_instance->reportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    
    bool initSuccess = false;
    try {
        initSuccess = s_instance->initialize();
    } catch (const std::exception& ex) {
        logToFile(QString("Exception in initialize(): %1").arg(ex.what()));
    } catch (...) {
        logToFile("Unknown exception in initialize()");
    }

    if (!initSuccess) {
        logToFile("Initialization failed.");
        s_instance->reportServiceStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }
    
    s_instance->reportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);
    
    std::thread workerThread(&WinService::serviceWorkerThread, s_instance);
    
    WaitForSingleObject(s_instance->m_serviceStopEvent, INFINITE);
    if (s_instance->m_serviceStopEvent == NULL) {
        logToFile("Service stop event is NULL");
        s_instance->reportServiceStatus(SERVICE_STOPPED, ERROR_INVALID_HANDLE, 0);
        return;
    }

    if (s_instance->m_appMonitor) {
        s_instance->m_appMonitor->stopMonitoring();
    }
    workerThread.join();
    
    s_instance->reportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

VOID WINAPI WinService::ServiceCtrlHandler(DWORD control)
{
    switch (control) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            if (s_instance) {
                s_instance->reportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);
                SetEvent(s_instance->m_serviceStopEvent);
            }
            reportServiceStatus(SERVICE_STOPPED);
            break;
        default:
            break;
    }
}

void WinService::reportServiceStatus(DWORD currentState, DWORD exitCode, DWORD waitHint)
{
    if (!s_instance) {
        logToFile("reportServiceStatus: s_instance is null");
        return;
    }

    static DWORD checkPoint = 1;
    
    m_serviceStatus.dwCurrentState = currentState;
    m_serviceStatus.dwWin32ExitCode = exitCode;
    m_serviceStatus.dwWaitHint = waitHint;

    logToFile(
        QString("Report service status: currentState %1 exitCode %2 waitHint %3")
            .arg(QString::number(currentState))
            .arg(QString::number(exitCode))
            .arg(QString::number(waitHint)));

    if (currentState == SERVICE_START_PENDING) {
        m_serviceStatus.dwControlsAccepted = 0;
    } else {
        m_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }
    
    if ((currentState == SERVICE_RUNNING) || (currentState == SERVICE_STOPPED)) {
        m_serviceStatus.dwCheckPoint = 0;
    } else {
        m_serviceStatus.dwCheckPoint = checkPoint++;
    }
    
    logToFile(
        QString("Report service status: currentState %1 exitCode %2 waitHint %3")
            .arg(QString::number(m_serviceStatus.dwCurrentState))
            .arg(QString::number(m_serviceStatus.dwWin32ExitCode))
            .arg(QString::number(m_serviceStatus.dwWaitHint)));

    SetServiceStatus(m_serviceStatusHandle, &m_serviceStatus);
}

void WinService::serviceWorkerThread()
{
    if (!m_appMonitor)
        return;

    // Start monitoring
    logToFile("Starting AppMonitor...");
    m_appMonitor->startMonitoring();
    logToFile("AppMonitor started: " + QString(m_appMonitor->isMonitoring() ? "true" : "false"));
    
    // Main service loop
    logToFile("Entering service main loop");
    int checkCounter = 0;
    while (WaitForSingleObject(m_serviceStopEvent, 0) != WAIT_OBJECT_0) {
        Sleep(1000);
        
        // Every 5 seconds, explicitly check running apps as a fallback
        // This helps in case the timer isn't working properly in service context
        // checkCounter++;
        // if (checkCounter >= 5) {
        //     QMetaObject::invokeMethod(m_appMonitor, "checkRunningApps", Qt::DirectConnection);
        //     checkCounter = 0;
        // }
    }
    
    logToFile("Service worker thread exiting");
} 