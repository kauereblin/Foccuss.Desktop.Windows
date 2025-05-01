#ifndef BLOCKTIMESETTINGSMODEL_H
#define BLOCKTIMESETTINGSMODEL_H

#include <QString>
#include <QMetaType>
#include <QTime>

struct REG_Week
{
  bool monday;
  bool tuesday;
  bool wednesday;
  bool thursday;
  bool friday;
  bool saturday;
  bool sunday;
};

class BlockTimeSettingsModel
{
public:
    BlockTimeSettingsModel(const QTime& startTime, const QTime& endTime, const REG_Week week, bool active);
    
    QTime getStartTime();
    QTime getEndTime();
    REG_Week getWeek();
    bool getActive();

    void setStartTime(const QTime& startTime);
    void setEndTime(const QTime& endTime);
    void setWeek(const REG_Week& week);
    void setActive(const bool active);

private:
    QTime m_startTime;
    QTime m_endTime;
    REG_Week m_week = { 0 };
    bool m_active = true;
};

// Register BlockTimeSettingsModel for QVariant
Q_DECLARE_METATYPE(std::shared_ptr<BlockTimeSettingsModel>)

#endif // BLOCKTIMESETTINGSMODEL_H 