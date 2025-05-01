#include "applistmodel.h"
#include <QStandardItem>
#include <QIcon>

AppListModel::AppListModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

void AppListModel::setApps(const QList<std::shared_ptr<AppModel>>& apps)
{
    clear();
    
    for (const auto& app : apps) {
        QStandardItem *item = new QStandardItem();
        item->setText(app->getName());
        item->setIcon(app->getIcon());
        item->setData(QVariant::fromValue(app), Qt::UserRole + 1);
        appendRow(item);
    }
} 