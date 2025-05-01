#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QStandardItemModel>
#include <QList>
#include <memory>
#include "../data/appmodel.h"

class AppListModel : public QStandardItemModel
{
    Q_OBJECT
    
public:
    explicit AppListModel(QObject *parent = nullptr);
    
    void setApps(const QList<std::shared_ptr<AppModel>>& apps);
};

#endif // APPLISTMODEL_H 