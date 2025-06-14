#include "appmonitor.h"
#include "../data/database.h"
#include "../data/appmodel.h"

#include <QDebug>
#include <TlHelp32.h>
#include <Psapi.h>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QThread>

static QString s_logFilePath;

void logToFile_(const QString& message)
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

AppMonitor::AppMonitor(Database* database, QObject *parent)
    : QObject(parent),
      m_database(database),
      m_isMonitoring(false)
{
    m_monitorTimer.setInterval(1000);
    connect(&m_monitorTimer, &QTimer::timeout, this, &AppMonitor::checkRunningApps);
}

void AppMonitor::startMonitoring()
{
    if (!m_isMonitoring && m_database && m_database->isInitialized()) {
        m_monitorTimer.start();
        m_isMonitoring = true;
        m_HwndCache.clear();
        
        QTimer::singleShot(0, this, &AppMonitor::checkRunningApps);
    } else {
        if (m_isMonitoring) {
            logToFile_("Monitoring already active");
        } else if (!m_database) {
            logToFile_("Cannot start monitoring - database is null");
        } else if (!m_database->isInitialized()) {
            logToFile_("Cannot start monitoring - database not initialized");
        }
    }
}

void AppMonitor::stopMonitoring()
{
    if (m_isMonitoring) {
        m_monitorTimer.stop();
        m_isMonitoring = false;
        m_HwndCache.clear();
    }
}

bool AppMonitor::isMonitoring() const
{
    return m_isMonitoring;
}

void AppMonitor::checkRunningApps()
{
    if (!m_database || !m_database->isInitialized())
        return;

    if (!m_database->isBlockingActive() || !m_database->isBlockingNow())
        return;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return;
    
    QSet<HWND> currentActiveHwnds;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return;
    }

    do {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            WCHAR szProcessPath[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, NULL, szProcessPath, MAX_PATH)) {
                QString processPath = QString::fromWCharArray(szProcessPath);
                QString processName = QString::fromWCharArray(pe32.szExeFile);

                if (processPath.toLower().contains("foccuss")) {
                    CloseHandle(hProcess);
                    continue;
                }

                if (m_database->isAppBlocked(processPath)) {
                    HWND hwnd = GetTopWindow(NULL);
                    while (hwnd) {
                        DWORD processId;
                        GetWindowThreadProcessId(hwnd, &processId);

                        if (processId == pe32.th32ProcessID) {
                            currentActiveHwnds.insert(hwnd);

                            if (!m_HwndCache.contains(hwnd)) {
                                m_HwndCache.insert(hwnd);

                                emit blockedAppLaunched(hwnd, processPath, processName);
                                QThread::msleep(100);
                            }
                        }
                        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
                    }
                }
            }
            CloseHandle(hProcess);
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    QSet<HWND> hwndsToRemove = m_HwndCache;
    hwndsToRemove.subtract(currentActiveHwnds);
    
    if (!hwndsToRemove.empty()) {
        for (const auto& hwnd : hwndsToRemove) {
            m_HwndCache.remove(hwnd);
        }
    }
}