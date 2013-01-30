#ifndef PORTGROUPSLIDER_H
#define PORTGROUPSLIDER_H

#include "labelledslider.h"
#include <QtGui>

class PortgroupSlider : public LabelledSlider
{
  Q_OBJECT
public:
  explicit PortgroupSlider(QString label, QWidget *parent = 0);
  
signals:
  void addEffects(QString portname);
  void removeEffects(QString portname);
public slots:
  void addEffects();
  void removeEffects();
private:
  QString portname;
};

#endif // PORTGROUPSLIDER_H
