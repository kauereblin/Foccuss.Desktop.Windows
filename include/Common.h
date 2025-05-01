#ifndef COMMON_H
#define COMMON_H

// Include our version check
#include "QtVersionCheck.h"

// Qt Core headers
#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTimer>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QSharedMemory>

// Qt GUI headers
#include <QIcon>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QTabWidget>
#include <QTimeEdit>
#include <QCheckBox>

// Qt Widgets headers
#include <QMainWindow>
#include <QWidget>
#include <QDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QListView>
#include <QListWidget>
#include <QTreeView>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>

// Qt SQL
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

// STL
#include <memory>
#include <algorithm>
#include <vector>
#include <string>
#include <functional>

// Windows specific
#ifdef Q_OS_WIN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#endif

#endif // COMMON_H 