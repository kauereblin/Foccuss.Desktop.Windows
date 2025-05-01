#include "appmodel.h"

#include <QFileInfo>
#include <QFileIconProvider>
#include <QProcess>
#include <Windows.h>
#include <TlHelp32.h>
#include <QDebug>

AppModel::AppModel(const QString& path, const QString& name, const bool active)
    : m_path(path), m_name(name), m_active(active)
{
    if (!m_path.isEmpty()) {
        loadIcon();
    }
}

QString AppModel::getPath() const
{
    return m_path;
}

QString AppModel::getName() const
{
    return m_name;
}

QIcon AppModel::getIcon() const
{
    return m_icon;
}

bool AppModel::getActive() const
{
    return m_active;
}

void AppModel::setPath(const QString& path)
{
    m_path = path;
    loadIcon();
}

void AppModel::setName(const QString& name)
{
    m_name = name;
}

bool AppModel::isValid() const
{
    return !m_path.isEmpty() && QFileInfo(m_path).exists();
}

void AppModel::setActive(const bool active)
{
    m_active = active;
}

bool AppModel::isRunning() const
{
    if (!isValid()) {
        return false;
    }
    
    // Extract executable name from path
    QFileInfo fileInfo(m_path);
    QString exeName = fileInfo.fileName().toLower();
    
    // Create a snapshot of the processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    // Get the first process
    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return false;
    }
    
    // Iterate through all processes
    do {
        QString processName = QString::fromWCharArray(pe32.szExeFile).toLower();
        if (processName == exeName) {
            CloseHandle(hSnapshot);
            return true;
        }
    } while (Process32Next(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    return false;
}

void AppModel::loadIcon()
{
    QFileInfo fileInfo(m_path);
    if (fileInfo.exists()) {
        QFileIconProvider iconProvider;
        m_icon = iconProvider.icon(fileInfo);
    }
} 