#ifndef APPMODEL_H
#define APPMODEL_H

#include <QString>
#include <QIcon>
#include <QMetaType>

class AppModel
{
public:
    AppModel(const QString& path, const QString& name, const bool active = false);

    QString getPath() const;
    QString getName() const;
    QIcon getIcon() const;
    bool getActive() const;
    
    void setPath(const QString& path);
    void setName(const QString& name);
    void setActive(const bool active);

    bool isValid() const;
    bool isRunning() const;

private:
    void loadIcon();

    QString m_path;
    QString m_name;
    QIcon m_icon;
    bool m_active = false;
};

// Register AppModel for QVariant
Q_DECLARE_METATYPE(std::shared_ptr<AppModel>)

#endif // APPMODEL_H 