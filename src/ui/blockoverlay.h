#ifndef BLOCKOVERLAY_H
#define BLOCKOVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <Windows.h>

class BlockOverlay : public QWidget
{
    Q_OBJECT
    
public:
    explicit BlockOverlay(const HWND targetWindow, const QString& appPath, const QString& appName, QWidget *parent = nullptr);
    ~BlockOverlay();
    
    void showOverWindow();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    
private slots:
    void onCloseClicked();
    void onKillAppClicked();
    void onCheckWindowPosition();
    
private:
    HWND findTargetWindow();
    void updatePosition();
    
    QString m_appPath;
    QString m_appName;
    QLabel *m_messageLabel;
    QLabel *m_appNameLabel;
    QPushButton *m_closeButton;
    QPushButton *m_killButton;
    
    HWND m_targetHwnd;
    QTimer m_positionTimer;
    bool m_isClosing;
};

#endif // BLOCKOVERLAY_H 