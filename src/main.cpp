#include "../include/Common.h"
#include "../include/ForwardDeclarations.h"

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QSharedMemory>
#include <QSqlDatabase>
#include <QSqlError>
#include <QPluginLoader>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <Windows.h>
#include <shellapi.h>

#include "ui/mainwindow.h"
#include "data/database.h"
#include "service/winservice.h"

bool isRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }
    return isAdmin;
}

bool requestAdminPrivileges() {
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(NULL, path, MAX_PATH)) {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = path;
        sei.lpParameters = L"--elevated";
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;
        
        if (ShellExecuteExW(&sei)) {
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Foccuss");
    app.setOrganizationName("Foccuss");
    app.setOrganizationDomain("foccuss.app");

    // Check if running as service
    if (argc > 1 && QString(argv[1]) == "--service") {
        // Initialize database
        Database* database = new Database();
        if (!database->initialize()) {
            qDebug() << "Failed to initialize database";
            delete database;
            return 1;
        }

        WinService service(database);
        if (!service.initialize()) {
            qDebug() << "Failed to initialize service";
            delete database;
            return 1;
        }
        
        // Register service control dispatcher
        SERVICE_TABLE_ENTRY serviceTable[] = {
            { (LPWSTR)service.getServiceName().utf16(), WinService::ServiceMain },
            { NULL, NULL }
        };
        
        if (!StartServiceCtrlDispatcher(serviceTable)) {
            DWORD error = GetLastError();
            qDebug() << "StartServiceCtrlDispatcher failed with error:" << error;
            delete database;
            return error;
        }
        
        return app.exec();
    }

    // Check if running elevated
    if (argc > 1 && QString(argv[1]) == "--elevated") {
        // Continue with elevated privileges
    } else if (!isRunningAsAdmin()) {
        // Request elevation
        if (requestAdminPrivileges()) {
            return 0; // Exit current instance
        } else {
            QMessageBox::critical(nullptr, "Foccuss", 
                "This application requires administrator privileges to install and manage the service.");
            return 1;
        }
    }

    // Ensure only one instance of the application is running
    QSharedMemory sharedMemory("FoccussAppInstance");
    if (!sharedMemory.create(1)) {
        QMessageBox::warning(nullptr, "Foccuss", "An instance of Foccuss is already running.");
        return 1;
    }
    
    // Load application stylesheet
    QFile styleFile(":/styles/main_style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }
    
    // Load SQLite driver
    QPluginLoader loader;
    loader.setFileName("./sqldrivers/qsqlite.dll");
    if (!loader.load()) {
        QMessageBox::critical(nullptr, "Foccuss", 
            QString("Failed to load SQLite driver: %1").arg(loader.errorString()));
        return 1;
    }
    
    // Initialize database
    Database* database = new Database();
    if (!database->initialize()) {
        QMessageBox::critical(nullptr, "Foccuss", "Failed to initialize database.");
        delete database;
        return 1;
    }

    // Install and start the service
    WinService service(database);
    if (!service.isServiceInstalled()) {
        if (!service.installService()) {
            QMessageBox::warning(nullptr, "Foccuss", 
                "Failed to install the service. The application will run without service support.");
        } else {
            if (!service.startService()) {
                QMessageBox::warning(nullptr, "Foccuss", 
                    "Failed to start the service. The application will run without service support.");
            }
        }
    }
    
    // Create and show main window
    MainWindow mainWindow(database);
    mainWindow.show();
    
    return app.exec();
} 