#include "mainwindow.h"
#include "blockoverlay.h"
#include "applistmodel.h"
#include "../core/appdetector.h"
#include "../core/appmonitor.h"
#include "../data/database.h"
#include "../data/appmodel.h"
#include "../data/blockTimeSettingsModel.h"
#include "../service/winservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QStandardItemModel>
#include <QListWidget>
#include <QMessageBox>
#include <QCloseEvent>
#include <QIcon>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QDebug>
#include <QApplication>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QLineEdit>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QRegularExpression>

MainWindow::MainWindow(Database* database, QWidget *parent)
    : QMainWindow(parent),
      m_installedAppsView(nullptr),
      m_blockedAppsView(nullptr),
      m_installedSearchEdit(nullptr),
      m_blockedSearchEdit(nullptr),
      m_refreshButton(nullptr),
      m_blockButton(nullptr),
      m_unblockButton(nullptr),
      m_statusLabel(nullptr),
      m_serviceToggleAction(nullptr),
      m_trayIcon(nullptr),
      m_trayMenu(nullptr),
      m_appDetector(nullptr),
      m_appMonitor(nullptr),
      m_database(database),
      m_service(nullptr)
{
    m_appDetector = new AppDetector(this);
    
    m_appMonitor = new AppMonitor(m_database, this);
    connect(m_appMonitor, &AppMonitor::blockedAppLaunched, this, &MainWindow::onBlockedAppLaunched);
    
    m_filteredInstalledApps.clear();
    m_filteredBlockedApps.clear();
    
    setupUi();
    setupTrayIcon();
    
    loadBlockedApps();

    m_appMonitor->startMonitoring();

    updateServiceStatus();

    setWindowTitle("Foccuss");
    setMinimumSize(800, 600);
}

MainWindow::~MainWindow()
{
    if (m_appMonitor) {
        m_appMonitor->stopMonitoring();
        delete m_appMonitor;
    }
    if (m_appDetector) {
        delete m_appDetector;
    }
    if (m_service) {
        delete m_service;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        QMessageBox::information(this, "Foccuss",
                               "Foccuss will continue running in the background.\n"
                               "To exit completely, right-click the tray icon and select Exit.");
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Create tabs
    m_appsTab = new QWidget();
    m_settingsTab = new QWidget();
    
    // Setup each tab
    setupAppsTab();
    setupSettingsTab();
    
    // Add tabs to tab widget
    m_tabWidget->addTab(m_appsTab, "Applications");
    m_tabWidget->addTab(m_settingsTab, "Settings");
    
    mainLayout->addWidget(m_tabWidget);
    
    m_service = new WinService(m_database, this);
    updateServiceButtons();
    
    // Load time settings
    loadTimeSettings();
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    
    QIcon icon = QIcon(":/icons/app_icon.png");
    if (icon.isNull()) {
        icon = QIcon::fromTheme("application-x-executable");
    }
    m_trayIcon->setIcon(icon);
    
    m_trayMenu = new QMenu(this);
    
    QAction *showHideAction = new QAction("Show/Hide", this);
    connect(showHideAction, &QAction::triggered, this, [this]() {
        if (isVisible()) {
            hide();
        } else {
            show();
            activateWindow();
        }
    });
    
    m_serviceToggleAction = new QAction("Enable Monitoring", this);
    m_serviceToggleAction->setCheckable(true);
    m_serviceToggleAction->setChecked(true);
    connect(m_serviceToggleAction, &QAction::triggered, this, &MainWindow::onServiceStatusToggled);

    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayMenu->addAction(showHideAction);
    m_trayMenu->addAction(m_serviceToggleAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(exitAction);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
}

void MainWindow::setupAppsTab()
{
    QVBoxLayout *tabLayout = new QVBoxLayout(m_appsTab);
    
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    
    QGroupBox *installedGroup = new QGroupBox("Installed Applications", this);
    QVBoxLayout *installedLayout = new QVBoxLayout(installedGroup);
    
    QHBoxLayout *installedSearchLayout = new QHBoxLayout();
    QLabel *installedSearchLabel = new QLabel("Search:", this);
    m_installedSearchEdit = new QLineEdit(this);
    m_installedSearchEdit->setPlaceholderText("Type to search applications...");
    connect(m_installedSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onInstalledAppsSearchChanged);
    installedSearchLayout->addWidget(installedSearchLabel);
    installedSearchLayout->addWidget(m_installedSearchEdit);
    installedLayout->addLayout(installedSearchLayout);
    
    m_installedAppsView = new QListView(this);
    m_installedAppsView->setModel(new AppListModel(this));
    m_refreshButton = new QPushButton("Refresh", this);
    m_blockButton = new QPushButton("Block", this);
    m_blockButton->setEnabled(false);
    
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshApps);
    connect(m_blockButton, &QPushButton::clicked, this, &MainWindow::onBlockApp);
    connect(m_installedAppsView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &selected) {
                if (selected.indexes().isEmpty()) {
                    m_selectedInstalledApp = nullptr;
                } else {
                    int row = selected.indexes().first().row();
                    if (row >= 0 && row < m_filteredInstalledApps.size()) {
                        m_selectedInstalledApp = m_filteredInstalledApps[row];
                    } else {
                        m_selectedInstalledApp = nullptr;
                    }
                }
                m_blockButton->setEnabled(m_selectedInstalledApp != nullptr);
            });
    
    QHBoxLayout *installedButtonsLayout = new QHBoxLayout();
    installedButtonsLayout->addWidget(m_refreshButton);
    installedButtonsLayout->addWidget(m_blockButton);
    
    installedLayout->addWidget(m_installedAppsView);
    installedLayout->addLayout(installedButtonsLayout);
    
    QGroupBox *blockedGroup = new QGroupBox("Blocked Applications", this);
    QVBoxLayout *blockedLayout = new QVBoxLayout(blockedGroup);
    
    QHBoxLayout *blockedSearchLayout = new QHBoxLayout();
    QLabel *blockedSearchLabel = new QLabel("Search:", this);
    m_blockedSearchEdit = new QLineEdit(this);
    m_blockedSearchEdit->setPlaceholderText("Type to search blocked applications...");
    connect(m_blockedSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onBlockedAppsSearchChanged);
    blockedSearchLayout->addWidget(blockedSearchLabel);
    blockedSearchLayout->addWidget(m_blockedSearchEdit);
    blockedLayout->addLayout(blockedSearchLayout);
    
    m_blockedAppsView = new QListView(this);
    m_blockedAppsView->setModel(new AppListModel(this));
    m_unblockButton = new QPushButton("Unblock", this);
    m_unblockButton->setEnabled(false);
    
    connect(m_unblockButton, &QPushButton::clicked, this, &MainWindow::onUnblockApp);
    connect(m_blockedAppsView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &selected) {
                if (selected.indexes().isEmpty()) {
                    m_selectedBlockedApp = nullptr;
                } else {
                    int row = selected.indexes().first().row();
                    if (row >= 0 && row < m_filteredBlockedApps.size()) {
                        m_selectedBlockedApp = m_filteredBlockedApps[row];
                    } else {
                        m_selectedBlockedApp = nullptr;
                    }
                }
                m_unblockButton->setEnabled(m_selectedBlockedApp != nullptr);
            });
    
    blockedLayout->addWidget(m_blockedAppsView);
    blockedLayout->addWidget(m_unblockButton);
    
    splitter->addWidget(installedGroup);
    splitter->addWidget(blockedGroup);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    
    tabLayout->addWidget(splitter);
}

void MainWindow::setupSettingsTab()
{
    QVBoxLayout *tabLayout = new QVBoxLayout(m_settingsTab);
    
    // Service Management Group
    QGroupBox *serviceGroup = new QGroupBox("Service Management", this);
    QHBoxLayout *serviceLayout = new QHBoxLayout(serviceGroup);
    serviceLayout->setContentsMargins(10, 5, 10, 5);
    
    m_statusLabel = new QLabel("Status: Service not installed", this);
    serviceLayout->addWidget(m_statusLabel);
    
    QSpacerItem *serviceSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    serviceLayout->addItem(serviceSpacer);
    
    m_installServiceButton = new QPushButton("Install", this);
    m_uninstallServiceButton = new QPushButton("Uninstall", this);
    m_startServiceButton = new QPushButton("Start", this);
    m_stopServiceButton = new QPushButton("Stop", this);
    
    connect(m_installServiceButton, &QPushButton::clicked, this, &MainWindow::onInstallService);
    connect(m_uninstallServiceButton, &QPushButton::clicked, this, &MainWindow::onUninstallService);
    connect(m_startServiceButton, &QPushButton::clicked, this, &MainWindow::onStartService);
    connect(m_stopServiceButton, &QPushButton::clicked, this, &MainWindow::onStopService);
    
    serviceLayout->addWidget(m_installServiceButton);
    serviceLayout->addWidget(m_uninstallServiceButton);
    serviceLayout->addWidget(m_startServiceButton);
    serviceLayout->addWidget(m_stopServiceButton);
    
    tabLayout->addWidget(serviceGroup);
    
    // Time Settings Group
    QGroupBox *timeGroup = new QGroupBox("Blocking Time Settings", this);
    QVBoxLayout *timeLayout = new QVBoxLayout(timeGroup);
    
    // Time Range
    QHBoxLayout *timeRangeLayout = new QHBoxLayout();
    QLabel *startTimeLabel = new QLabel("Start Time:", this);
    m_startTimeEdit = new QTimeEdit(this);
    m_startTimeEdit->setDisplayFormat("HH:mm");
    
    QLabel *endTimeLabel = new QLabel("End Time:", this);
    m_endTimeEdit = new QTimeEdit(this);
    m_endTimeEdit->setDisplayFormat("HH:mm");
    
    timeRangeLayout->addWidget(startTimeLabel);
    timeRangeLayout->addWidget(m_startTimeEdit);
    timeRangeLayout->addSpacerItem(new QSpacerItem(20, 10, QSizePolicy::Fixed, QSizePolicy::Minimum));
    timeRangeLayout->addWidget(endTimeLabel);
    timeRangeLayout->addWidget(m_endTimeEdit);
    timeRangeLayout->addStretch();
    
    // Days of Week
    QGroupBox *daysGroup = new QGroupBox("Active Days", this);
    QGridLayout *daysLayout = new QGridLayout(daysGroup);
    
    m_mondayCheckBox = new QCheckBox("Monday", this);
    m_tuesdayCheckBox = new QCheckBox("Tuesday", this);
    m_wednesdayCheckBox = new QCheckBox("Wednesday", this);
    m_thursdayCheckBox = new QCheckBox("Thursday", this);
    m_fridayCheckBox = new QCheckBox("Friday", this);
    m_saturdayCheckBox = new QCheckBox("Saturday", this);
    m_sundayCheckBox = new QCheckBox("Sunday", this);
    
    daysLayout->addWidget(m_mondayCheckBox, 0, 0);
    daysLayout->addWidget(m_tuesdayCheckBox, 0, 1);
    daysLayout->addWidget(m_wednesdayCheckBox, 0, 2);
    daysLayout->addWidget(m_thursdayCheckBox, 1, 0);
    daysLayout->addWidget(m_fridayCheckBox, 1, 1);
    daysLayout->addWidget(m_saturdayCheckBox, 1, 2);
    daysLayout->addWidget(m_sundayCheckBox, 2, 0);
    
    // Active Status
    QHBoxLayout *activeLayout = new QHBoxLayout();
    m_blockingActiveCheckBox = new QCheckBox("Enable Blocking Schedule", this);
    activeLayout->addWidget(m_blockingActiveCheckBox);
    activeLayout->addStretch();
    
    // Save Button
    QHBoxLayout *saveLayout = new QHBoxLayout();
    m_saveSettingsButton = new QPushButton("Save Settings", this);
    connect(m_saveSettingsButton, &QPushButton::clicked, this, &MainWindow::onSaveTimeSettings);
    saveLayout->addStretch();
    saveLayout->addWidget(m_saveSettingsButton);
    
    // Add everything to time layout
    timeLayout->addLayout(timeRangeLayout);
    timeLayout->addWidget(daysGroup);
    timeLayout->addLayout(activeLayout);
    timeLayout->addLayout(saveLayout);
    
    tabLayout->addWidget(timeGroup);
    tabLayout->addStretch();
}

void MainWindow::loadBlockedApps()
{
    m_blockedApps = m_database->getBlockedApps();

    if (m_blockedSearchEdit && !m_blockedSearchEdit->text().isEmpty()) {
        filterAppList(m_blockedSearchEdit->text(), false);
    } else {
        m_filteredBlockedApps = m_blockedApps;
    
        AppListModel *model = qobject_cast<AppListModel*>(m_blockedAppsView->model());
        if (model) {
            model->setApps(m_filteredBlockedApps);
        }
    }
}

void MainWindow::updateServiceStatus()
{
    bool isMonitoring = m_appMonitor->isMonitoring();
    
    m_statusLabel->setText(isMonitoring 
                          ? "Status: Monitoring is active" 
                          : "Status: Monitoring is inactive");
                          
    if (m_serviceToggleAction) {
        m_serviceToggleAction->setChecked(isMonitoring);
        m_serviceToggleAction->setText(isMonitoring 
                                     ? "Disable Monitoring" 
                                     : "Enable Monitoring");
    }
}

void MainWindow::onRefreshApps()
{
    m_appDetector->refreshInstalledApps();
    m_installedApps = m_appDetector->getInstalledApps();
    
    if (m_installedSearchEdit && !m_installedSearchEdit->text().isEmpty()) {
        filterAppList(m_installedSearchEdit->text(), true);
    } else {
        m_filteredInstalledApps = m_installedApps;
        
        AppListModel *model = qobject_cast<AppListModel*>(m_installedAppsView->model());
        if (model) {
            model->setApps(m_filteredInstalledApps);
        }
    }
    
    m_selectedInstalledApp = nullptr;
    m_blockButton->setEnabled(false);
}

void MainWindow::onBlockApp()
{
    if (m_selectedInstalledApp && m_selectedInstalledApp->isValid()) {
        if (m_database->addBlockedApp(m_selectedInstalledApp->getPath(), 
                                     m_selectedInstalledApp->getName())) {
            loadBlockedApps();
            
            QMessageBox::information(this, "Success", 
                                   "Application has been blocked: " + 
                                   m_selectedInstalledApp->getName());
        } else {
            QMessageBox::warning(this, "Error", 
                               "Failed to block application: " + 
                               m_selectedInstalledApp->getName());
        }
    }
}

void MainWindow::onUnblockApp()
{
    if (m_selectedBlockedApp && m_selectedBlockedApp->isValid()) {
        if (m_database->removeBlockedApp(m_selectedBlockedApp->getPath())) {
            loadBlockedApps();
            
            QMessageBox::information(this, "Success", 
                                   "Application has been unblocked: " + 
                                   m_selectedBlockedApp->getName());
        } else {
            QMessageBox::warning(this, "Error", 
                               "Failed to unblock application: " + 
                               m_selectedBlockedApp->getName());
        }
    }
}

void MainWindow::onAppSelected(const QModelIndex &index)
{
    QAbstractItemModel *model = const_cast<QAbstractItemModel*>(index.model());
    if (!model) {
        return;
    }
    
    QStandardItemModel *stdModel = dynamic_cast<QStandardItemModel*>(model);
    if (!stdModel) {
        return;
    }
    
    QStandardItem *item = stdModel->itemFromIndex(index);
    if (!item) {
        return;
    }
    
    QVariant appVariant = item->data(Qt::UserRole + 1);
    if (appVariant.isValid()) {
        std::shared_ptr<AppModel> app = appVariant.value<std::shared_ptr<AppModel>>();
        
        if (sender() == m_installedAppsView) {
            m_selectedInstalledApp = app;
            m_blockButton->setEnabled(true);
            m_unblockButton->setEnabled(false);
        } else if (sender() == m_blockedAppsView) {
            m_selectedBlockedApp = app;
            m_blockButton->setEnabled(false);
            m_unblockButton->setEnabled(true);
        }
    }
}

void MainWindow::onBlockedAppLaunched(const HWND targetWindow, const QString& appPath, const QString& appName)
{
    BlockOverlay *overlay = new BlockOverlay(targetWindow, appPath, appName);
    overlay->setAttribute(Qt::WA_DeleteOnClose);
    overlay->showOverWindow();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            show();
            activateWindow();
        }
    }
}

void MainWindow::onServiceStatusToggled(bool checked)
{
    if (checked) {
        m_appMonitor->startMonitoring();
    } else {
        m_appMonitor->stopMonitoring();
    }

    updateServiceStatus();
}

void MainWindow::updateServiceButtons()
{
    bool isInstalled = m_service->isServiceInstalled();
    bool isRunning = m_service->isServiceRunning();
    
    m_installServiceButton->setEnabled(!isInstalled);
    m_uninstallServiceButton->setEnabled(isInstalled);
    m_startServiceButton->setEnabled(isInstalled && !isRunning);
    m_stopServiceButton->setEnabled(isInstalled && isRunning);
    
    QString statusText = "Status: ";
    if (!isInstalled) {
        statusText += "Service not installed";
    } else if (isRunning) {
        statusText += "Service is running";
    } else {
        statusText += "Service is stopped";
    }
    m_statusLabel->setText(statusText);
}

void MainWindow::onInstallService()
{
    if (m_service->installService()) {
        QMessageBox::information(this, "Success", "Service installed successfully");
        updateServiceButtons();
    } else {
        QMessageBox::critical(this, "Error", "Failed to install service");
    }
}

void MainWindow::onUninstallService()
{
    if (m_service->uninstallService()) {
        QMessageBox::information(this, "Success", "Service uninstalled successfully");
        updateServiceButtons();
    } else {
        QMessageBox::critical(this, "Error", "Failed to uninstall service");
    }
}

void MainWindow::onStartService()
{
    if (m_service->startService()) {
        QMessageBox::information(this, "Success", "Service started successfully");
        updateServiceButtons();
    } else {
        QMessageBox::critical(this, "Error", "Failed to start service");
    }
}

void MainWindow::onStopService()
{
    if (m_service->stopService()) {
        QMessageBox::information(this, "Success", "Service stopped successfully");
        updateServiceButtons();
    } else {
        QMessageBox::critical(this, "Error", "Failed to stop service");
    }
}

void MainWindow::onInstalledAppsSearchChanged(const QString& text)
{
    if (m_installedApps.isEmpty() && text.isEmpty()) {
        return;
    }
    
    if (m_installedApps.isEmpty() && !text.isEmpty()) {
        onRefreshApps();
    }
    
    filterAppList(text, true);
}

void MainWindow::onBlockedAppsSearchChanged(const QString& text)
{
    if (!m_blockedApps.isEmpty() || text.isEmpty()) {
        filterAppList(text, false);
    }
}

void MainWindow::onSaveTimeSettings()
{
    if (!m_timeSettings) {
        REG_Week week = {
            m_mondayCheckBox->isChecked(),
            m_tuesdayCheckBox->isChecked(),
            m_wednesdayCheckBox->isChecked(),
            m_thursdayCheckBox->isChecked(),
            m_fridayCheckBox->isChecked(),
            m_saturdayCheckBox->isChecked(),
            m_sundayCheckBox->isChecked()
        };
        
        m_timeSettings = std::make_shared<BlockTimeSettingsModel>(
            m_startTimeEdit->time(), 
            m_endTimeEdit->time(),
            week,
            m_blockingActiveCheckBox->isChecked()
        );
    } else {
        // Update existing settings
        m_timeSettings->setStartTime(m_startTimeEdit->time());
        m_timeSettings->setEndTime(m_endTimeEdit->time());
        
        REG_Week week = {
            m_mondayCheckBox->isChecked(),
            m_tuesdayCheckBox->isChecked(),
            m_wednesdayCheckBox->isChecked(),
            m_thursdayCheckBox->isChecked(),
            m_fridayCheckBox->isChecked(),
            m_saturdayCheckBox->isChecked(),
            m_sundayCheckBox->isChecked()
        };
        m_timeSettings->setWeek(week);
        m_timeSettings->setActive(m_blockingActiveCheckBox->isChecked());
    }
    
    // Save to database
    if (m_database->updateBlockTimeSettings(m_timeSettings)) {
        QMessageBox::information(this, "Success", "Time settings saved successfully");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save time settings");
    }
}

void MainWindow::filterAppList(const QString& searchText, bool isInstalledList)
{
    QList<std::shared_ptr<AppModel>>& sourceList = isInstalledList ? m_installedApps : m_blockedApps;
    QList<std::shared_ptr<AppModel>>& filteredList = isInstalledList ? m_filteredInstalledApps : m_filteredBlockedApps;
    QListView* listView = isInstalledList ? m_installedAppsView : m_blockedAppsView;

    filteredList.clear();

    if (searchText.isEmpty()) {
        filteredList = sourceList;
    } else {
        QString pattern = searchText;
        pattern.replace(" ", "*");
        if (!pattern.startsWith("*")) pattern = "*" + pattern;
        if (!pattern.endsWith("*")) pattern = pattern + "*";
        
        QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(pattern), 
                                QRegularExpression::CaseInsensitiveOption);
        
        for (const auto& app : sourceList) {
            if (regex.match(app->getName()).hasMatch()) {
                filteredList.append(app);
            }
        }
    }

    AppListModel* model = qobject_cast<AppListModel*>(listView->model());
    if (model) {
        model->setApps(filteredList);
    }
}

void MainWindow::loadTimeSettings()
{
    m_timeSettings = m_database->getBlockTimeSettings();
    
    // If settings exist, populate UI elements
    if (m_timeSettings) {
        m_startTimeEdit->setTime(m_timeSettings->getStartTime());
        m_endTimeEdit->setTime(m_timeSettings->getEndTime());
        
        REG_Week week = m_timeSettings->getWeek();
        m_mondayCheckBox->setChecked(week.monday);
        m_tuesdayCheckBox->setChecked(week.tuesday);
        m_wednesdayCheckBox->setChecked(week.wednesday);
        m_thursdayCheckBox->setChecked(week.thursday);
        m_fridayCheckBox->setChecked(week.friday);
        m_saturdayCheckBox->setChecked(week.saturday);
        m_sundayCheckBox->setChecked(week.sunday);
        
        m_blockingActiveCheckBox->setChecked(m_timeSettings->getActive());
    } else {
        // Default values if no settings exist
        m_startTimeEdit->setTime(QTime(8, 0));
        m_endTimeEdit->setTime(QTime(17, 0));
        
        m_mondayCheckBox->setChecked(true);
        m_tuesdayCheckBox->setChecked(true);
        m_wednesdayCheckBox->setChecked(true);
        m_thursdayCheckBox->setChecked(true);
        m_fridayCheckBox->setChecked(true);
        m_saturdayCheckBox->setChecked(false);
        m_sundayCheckBox->setChecked(false);
        
        m_blockingActiveCheckBox->setChecked(true);
        
        // Create default settings model
        REG_Week defaultWeek = {true, true, true, true, true, false, false};
        m_timeSettings = std::make_shared<BlockTimeSettingsModel>(
            QTime(8, 0), QTime(17, 0), defaultWeek, true);
    }
}

void MainWindow::saveTimeSettings()
{
    if (!m_timeSettings) {
        REG_Week week = {
            m_mondayCheckBox->isChecked(),
            m_tuesdayCheckBox->isChecked(),
            m_wednesdayCheckBox->isChecked(),
            m_thursdayCheckBox->isChecked(),
            m_fridayCheckBox->isChecked(),
            m_saturdayCheckBox->isChecked(),
            m_sundayCheckBox->isChecked()
        };
        
        m_timeSettings = std::make_shared<BlockTimeSettingsModel>(
            m_startTimeEdit->time(), 
            m_endTimeEdit->time(),
            week,
            m_blockingActiveCheckBox->isChecked()
        );
    } else {
        // Update existing settings
        m_timeSettings->setStartTime(m_startTimeEdit->time());
        m_timeSettings->setEndTime(m_endTimeEdit->time());
        
        REG_Week week = {
            m_mondayCheckBox->isChecked(),
            m_tuesdayCheckBox->isChecked(),
            m_wednesdayCheckBox->isChecked(),
            m_thursdayCheckBox->isChecked(),
            m_fridayCheckBox->isChecked(),
            m_saturdayCheckBox->isChecked(),
            m_sundayCheckBox->isChecked()
        };
        m_timeSettings->setWeek(week);
        m_timeSettings->setActive(m_blockingActiveCheckBox->isChecked());
    }
    
    // Save to database
    if (m_database->updateBlockTimeSettings(m_timeSettings)) {
        QMessageBox::information(this, "Success", "Time settings saved successfully");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save time settings");
    }
}

