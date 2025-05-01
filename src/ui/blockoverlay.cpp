#include "blockoverlay.h"
#include "../../include/Common.h"

#include <QFileInfo>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QCloseEvent>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

BlockOverlay::BlockOverlay(const HWND targetWindow, const QString& appPath, const QString& appName, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      m_appPath(appPath),
      m_appName(appName),
      m_targetHwnd(targetWindow),
      m_isClosing(false)
{
    setObjectName("blockOverlay");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);
    
    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName("blockMessageLabel");
    m_messageLabel->setText("This application has been blocked!");
    m_messageLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_messageLabel);
    
    m_appNameLabel = new QLabel(this);
    m_appNameLabel->setText(m_appName);
    m_appNameLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_appNameLabel);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    mainLayout->addLayout(buttonLayout);
    
    m_closeButton = new QPushButton("Close Overlay", this);
    connect(m_closeButton, &QPushButton::clicked, this, &BlockOverlay::onCloseClicked);
    
    m_killButton = new QPushButton("Force Close App", this);
    connect(m_killButton, &QPushButton::clicked, this, &BlockOverlay::onKillAppClicked);
    
    buttonLayout->addWidget(m_closeButton);
    buttonLayout->addWidget(m_killButton);
    
    setMinimumSize(400, 300);
    
    m_positionTimer.setInterval(100);
    connect(&m_positionTimer, &QTimer::timeout, this, &BlockOverlay::onCheckWindowPosition);
}

BlockOverlay::~BlockOverlay()
{
    m_positionTimer.stop();
}

void BlockOverlay::showOverWindow()
{
    if (                   m_targetHwnd  != NULL
        && IsWindow       (m_targetHwnd) == TRUE
        && IsWindowVisible(m_targetHwnd) == TRUE)
    {
        updatePosition();
        
        show();
        raise();
        activateWindow();
        
        m_positionTimer.start();
    }
}

void BlockOverlay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    
    painter.setOpacity(0.5);
    painter.setBrush(QColor(200, 30, 30, 128));
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());
    
    QWidget::paintEvent(event);
}

void BlockOverlay::closeEvent(QCloseEvent *event)
{
    m_positionTimer.stop();
    m_isClosing = true;
    event->accept();
}

bool BlockOverlay::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG* msg = static_cast<MSG*>(message);
    
    if (msg->message == WM_DISPLAYCHANGE || msg->message == WM_ACTIVATEAPP) {
        if (!m_isClosing         &&
            m_targetHwnd != NULL &&
            IsWindow(m_targetHwnd) == FALSE)
        {
            close();
            return true;
        }
    }
    
    return QWidget::nativeEvent(eventType, message, result);
}

void BlockOverlay::onCloseClicked()
{
    close();
}

void BlockOverlay::onKillAppClicked()
{
    if (m_targetHwnd != NULL) {
        DWORD processId;
        GetWindowThreadProcessId(m_targetHwnd, &processId);

        // Open the process with termination rights
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
        
        if (hProcess) {
            // Terminate the process
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }
    
    close();
}

void BlockOverlay::onCheckWindowPosition()
{
    if (m_targetHwnd == NULL || IsWindow(m_targetHwnd) == FALSE) {
        close();
        return;
    }
    
    if (IsWindowVisible(m_targetHwnd) == FALSE) {
        close();
        return;
    }

    updatePosition();

    if (GetForegroundWindow() == m_targetHwnd) {
        BringWindowToTop((HWND)winId());
        activateWindow();
    }
}

void BlockOverlay::updatePosition()
{
    if (m_targetHwnd != NULL)
    {
        RECT targetRect;
        GetWindowRect(m_targetHwnd, &targetRect);
        
        setGeometry(
            targetRect.left,
            targetRect.top,
            targetRect.right - targetRect.left,
            targetRect.bottom - targetRect.top
        );
    }
} 