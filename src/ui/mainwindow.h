#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../../include/Common.h"
#include "../../include/ForwardDeclarations.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Database* database, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onRefreshApps();
    void onBlockApp();
    void onUnblockApp();
    void onAppSelected(const QModelIndex &index);
    void onBlockedAppLaunched(const HWND targetWindow, const QString& appPath, const QString& appName);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onServiceStatusToggled(bool checked);
    void onInstallService();
    void onUninstallService();
    void onStartService();
    void onStopService();
    void onInstalledAppsSearchChanged(const QString& text);
    void onBlockedAppsSearchChanged(const QString& text);
    void onSaveTimeSettings();

private:
    void setupUi();
    void setupTrayIcon();
    void setupAppsTab();
    void setupSettingsTab();
    void loadBlockedApps();
    void updateServiceStatus();
    void updateServiceButtons();
    void filterAppList(const QString& searchText, bool isInstalledList);
    void loadTimeSettings();
    void saveTimeSettings();
    
private:
    QTabWidget* m_tabWidget;
    QWidget* m_appsTab;
    QWidget* m_settingsTab;
    
    QListView *m_installedAppsView;
    QListView *m_blockedAppsView;
    QLineEdit *m_installedSearchEdit;
    QLineEdit *m_blockedSearchEdit;
    QPushButton *m_refreshButton;
    QPushButton *m_blockButton;
    QPushButton *m_unblockButton;
    QPushButton *m_installServiceButton;
    QPushButton *m_uninstallServiceButton;
    QPushButton *m_startServiceButton;
    QPushButton *m_stopServiceButton;
    QLabel *m_statusLabel;
    QAction *m_serviceToggleAction;

    QTimeEdit* m_startTimeEdit;
    QTimeEdit* m_endTimeEdit;
    QCheckBox* m_mondayCheckBox;
    QCheckBox* m_tuesdayCheckBox;
    QCheckBox* m_wednesdayCheckBox;
    QCheckBox* m_thursdayCheckBox;
    QCheckBox* m_fridayCheckBox;
    QCheckBox* m_saturdayCheckBox;
    QCheckBox* m_sundayCheckBox;
    QCheckBox* m_blockingActiveCheckBox;
    QPushButton* m_saveSettingsButton;

    // System tray
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;

    // Models
    QList<std::shared_ptr<AppModel>> m_installedApps;
    QList<std::shared_ptr<AppModel>> m_blockedApps;
    QList<std::shared_ptr<AppModel>> m_filteredInstalledApps;
    QList<std::shared_ptr<AppModel>> m_filteredBlockedApps;

    // Core components
    AppDetector *m_appDetector;
    AppMonitor *m_appMonitor;
    Database *m_database;
    WinService *m_service;

    // Selected app
    std::shared_ptr<AppModel> m_selectedInstalledApp;
    std::shared_ptr<AppModel> m_selectedBlockedApp;
    std::shared_ptr<BlockTimeSettingsModel> m_timeSettings;
};

#endif // MAINWINDOW_H 