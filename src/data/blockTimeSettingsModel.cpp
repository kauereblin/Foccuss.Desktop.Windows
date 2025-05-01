#include "blockTimeSettingsModel.h"

BlockTimeSettingsModel::BlockTimeSettingsModel(const QTime& startTime, const QTime& endTime, const REG_Week week, bool active)
  : m_startTime(startTime), m_endTime(endTime)
{
  setWeek(week);
  setActive(active);
}

QTime BlockTimeSettingsModel::getStartTime()
{
  return m_startTime;
}

QTime BlockTimeSettingsModel::getEndTime()
{
  return m_endTime;
}

REG_Week BlockTimeSettingsModel::getWeek()
{
  return m_week;
}

bool BlockTimeSettingsModel::getActive()
{
  return m_active;
}

void BlockTimeSettingsModel::setStartTime(const QTime& startTime)
{
  m_startTime = startTime;
}

void BlockTimeSettingsModel::setEndTime(const QTime& endTime)
{
  m_endTime = endTime;
}

void BlockTimeSettingsModel::setWeek(const REG_Week& week)
{
    m_week = week;
}

void BlockTimeSettingsModel::setActive(const bool active)
{
  m_active = active;
}
