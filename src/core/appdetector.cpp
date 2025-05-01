#include "appdetector.h"
#include "../data/appmodel.h"

#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QFileInfo>
#include <QDebug>
#include <Windows.h>
#include <TlHelp32.h>
#include <QSet>
#include <Psapi.h>  // For GetModuleFileNameEx
#include <ShlObj.h>  // For Shell link interfaces
#include <comdef.h>  // For COM support
#include <objbase.h>  // For CoCreateInstance
#include <QRegularExpression>

AppDetector::AppDetector(QObject *parent) 
    : QObject(parent)
{
    refreshInstalledApps();
}

QList<std::shared_ptr<AppModel>> AppDetector::getInstalledApps() const
{
    return m_installedApps;
}

QList<std::shared_ptr<AppModel>> AppDetector::getRunningApps() const
{
    QList<std::shared_ptr<AppModel>> runningApps;
    
    // Create a snapshot of the processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return runningApps;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    // Get the first process
    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return runningApps;
    }
    
    do {
        QString processName = QString::fromWCharArray(pe32.szExeFile);
        
        // Skip system processes
        if (processName.toLower().startsWith("system") || 
            processName.toLower() == "explorer.exe" ||
            processName.toLower() == "foccuss.exe") {
            continue;
        }
        
        // Get process path
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            WCHAR szProcessPath[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, NULL, szProcessPath, MAX_PATH)) {
                QString processPath = QString::fromWCharArray(szProcessPath);
                runningApps.append(std::make_shared<AppModel>(processPath, processName, false));
            }
            CloseHandle(hProcess);
        }
    } while (Process32Next(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    return runningApps;
}

void AppDetector::refreshInstalledApps()
{
    m_installedApps.clear();
    
    findAppsInRegistry();
    
    std::sort(m_installedApps.begin(), m_installedApps.end(), 
              [](const std::shared_ptr<AppModel>& a, const std::shared_ptr<AppModel>& b) {
                  return a->getName().toLower() < b->getName().toLower();
              });
    
    emit installedAppsChanged();
}

void AppDetector::findAppsInRegistry()
{
    // Read installed applications from HKLM registry - 64-bit applications
    QSettings uninstallRegistry64("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 
                               QSettings::NativeFormat);
    
    // Read installed applications from HKLM registry - 32-bit applications on 64-bit Windows
    QSettings uninstallRegistry32("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 
                               QSettings::NativeFormat);
    
    // Read per-user installed applications
    QSettings uninstallRegistryUser("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 
                                 QSettings::NativeFormat);
    
    // Process each registry path
    processRegistryApps(uninstallRegistry64);
    processRegistryApps(uninstallRegistry32);
    processRegistryApps(uninstallRegistryUser);
}

void AppDetector::processRegistryApps(QSettings& registry)
{
    QStringList appKeys = registry.childGroups();
    
    for (const QString& appKey : appKeys) {
        registry.beginGroup(appKey);
        
        QString displayName = registry.value("DisplayName").toString();
        QString uninstallString = registry.value("UninstallString").toString();
        QString installLocation = registry.value("InstallLocation").toString();
        
        // Also try to get these additional useful keys
        QString displayIcon = registry.value("DisplayIcon").toString();
        QString publisher = registry.value("Publisher").toString();
        
        // Skip entries with empty display names or system components
        if (displayName.isEmpty() || registry.value("SystemComponent").toInt() == 1) {
            registry.endGroup();
            continue;
        }
        
        // Skip Windows Store apps (they don't have direct executables we can block)
        if (publisher.contains("Microsoft Corporation") && 
            displayName.startsWith("Microsoft") && 
            installLocation.isEmpty()) {
            registry.endGroup();
            continue;
        }
        
        // Try different methods to find the executable path
        QString exePath;
        
        // Method 1: Try to extract from DisplayIcon first - this often points to the main executable
        if (!displayIcon.isEmpty()) {
            exePath = extractExecutableFromDisplayIcon(displayIcon);
        }
        
        // Method 2: Check InstallLocation if we couldn't find from DisplayIcon
        if (exePath.isEmpty() && !installLocation.isEmpty()) {
            exePath = findExecutableInDirectory(installLocation, displayName);
        }
        
        // Method 3: Try to find the path from the app key name (sometimes contains path info)
        if (exePath.isEmpty()) {
            exePath = extractExecutableFromAppKey(appKey, displayName);
        }
        
        // Method 4: Extract from UninstallString as a last resort
        if (exePath.isEmpty() && !uninstallString.isEmpty()) {
            exePath = inferExecutableFromUninstallString(uninstallString, displayName);
        }
        
        // Method 5: Try looking for the app in common installation directories
        if (exePath.isEmpty()) {
            exePath = findCommonExecutablePath(displayName, appKey);
        }
        
        // If we found an executable, add it to our list
        if (!exePath.isEmpty()) {
            QString cleanDisplayName = displayName;
            // Remove any trailing version numbers in parentheses for cleaner display
            int parenthesisPos = cleanDisplayName.indexOf(" (");
            if (parenthesisPos > 0) {
                cleanDisplayName = cleanDisplayName.left(parenthesisPos);
            }
            
            m_installedApps.append(std::make_shared<AppModel>(exePath, cleanDisplayName, false));
        }
        
        registry.endGroup();
    }
}

QString AppDetector::findExecutableInDirectory(const QString& directory, const QString& appName)
{
    QDir dir(directory);
    if (!dir.exists())
        return QString();
    
    QString possibleExeName = appName.split(" ").first() + ".exe";
    if (dir.exists(possibleExeName))
        return dir.filePath(possibleExeName);
    
    QStringList exeFiles = dir.entryList(QStringList() << "*.exe", QDir::Files);
    if (!exeFiles.isEmpty())
        return dir.filePath(exeFiles.first());
    
    QStringList commonSubdirs = {"bin", "program", "app", "programs", "applications"};
    for (const QString& subdir : commonSubdirs) {
        QDir subdirectory(dir.filePath(subdir));
        if (subdirectory.exists()) {
            exeFiles = subdirectory.entryList(QStringList() << "*.exe", QDir::Files);
            if (!exeFiles.isEmpty())
                return subdirectory.filePath(exeFiles.first());
        }
    }
    
    return QString();
}

QString AppDetector::extractExecutableFromDisplayIcon(const QString& displayIcon)
{
    QString iconPath = displayIcon.split(",").first().trimmed();
    
    if (iconPath.startsWith("\"") && iconPath.endsWith("\"")) {
        iconPath = iconPath.mid(1, iconPath.length() - 2);
    }
    
    QFileInfo fileInfo(iconPath);
    if (fileInfo.exists() && fileInfo.suffix().toLower() == "exe") {
        return iconPath;
    }
    
    return QString();
}

QString AppDetector::extractExecutableFromAppKey(const QString& appKey, const QString& displayName)
{
    if (appKey.contains("\\")) {
        QStringList parts = appKey.split("\\");
        if (parts.size() >= 2) {
            QString possiblePath = parts.join("\\");
            if (QFileInfo(possiblePath).exists() && QFileInfo(possiblePath).suffix().toLower() == "exe") {
                return possiblePath;
            }
            
            QDir dir(possiblePath);
            if (dir.exists()) {
                return findExecutableInDirectory(possiblePath, displayName);
            }
        }
    }
    
    return QString();
}

QString AppDetector::inferExecutableFromUninstallString(const QString& uninstallString, const QString& displayName)
{
    QRegularExpression quoteRegex("\"([^\"]+)\"");
    QRegularExpressionMatch match = quoteRegex.match(uninstallString);
    
    if (match.hasMatch()) {
        QString uninstallerPath = match.captured(1);
        QFileInfo uninstallerInfo(uninstallerPath);
        
        if (!uninstallerInfo.exists()) {
            return QString();
        }
        
        QDir appDir = uninstallerInfo.dir();
        
        QStringList exeFiles = appDir.entryList(QStringList() << "*.exe", QDir::Files);
        for (const QString& exeFile : exeFiles) {
            QString lowerExe = exeFile.toLower();
            
            if (lowerExe.contains("unins") || 
                lowerExe.contains("setup") || 
                lowerExe.contains("install") ||
                lowerExe == "uninstall.exe") {
                continue;
            }
            
            QString simpleAppName = displayName.split(" ").first().toLower();
            if (lowerExe.contains(simpleAppName)) {
                return appDir.filePath(exeFile);
            }
        }
        
        QDir parentDir = appDir;
        if (parentDir.cdUp()) {
            QString result = findExecutableInDirectory(parentDir.absolutePath(), displayName);
            if (!result.isEmpty()) {
                return result;
            }
        }
        
        for (const QString& exeFile : exeFiles) {
            QString lowerExe = exeFile.toLower();
            if (!lowerExe.contains("unins") && 
                !lowerExe.contains("setup") && 
                !lowerExe.contains("install") &&
                lowerExe != "uninstall.exe") {
                return appDir.filePath(exeFile);
            }
        }
    }
    
    return QString();
}

QString AppDetector::findCommonExecutablePath(const QString& appName, const QString& appKey)
{
    QStringList programDirs;
    programDirs << QDir::fromNativeSeparators(qgetenv("ProgramFiles"))
                << QDir::fromNativeSeparators(qgetenv("ProgramFiles(x86)"));
    
    QString simplifiedName = appName.split(" ").first();
    
    for (const QString& programDir : programDirs) {
        QDir dir(programDir + "/" + simplifiedName);
        if (dir.exists()) {
            QString exePath = findExecutableInDirectory(dir.absolutePath(), simplifiedName);
            if (!exePath.isEmpty())
                return exePath;
        }
        
        QStringList dirs = QDir(programDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& dirname : dirs) {
            if (dirname.contains(simplifiedName, Qt::CaseInsensitive)) {
                QString exePath = findExecutableInDirectory(programDir + "/" + dirname, simplifiedName);
                if (!exePath.isEmpty())
                    return exePath;
            }
        }
    }
    
    return QString();
} 