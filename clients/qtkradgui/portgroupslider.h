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
  void xmms2Play(QString portname);
  void xmms2Pause(QString portname);
  void xmms2Stop(QString portname);
  void xmms2Next(QString portname);
  void xmms2Prev(QString portname);

public slots:
  void addEffects();
  void removeEffects();
  void fxToggled(int state);
  void addXmms2Controls();
  void xmms2Play();
  void xmms2Pause();
  void xmms2Stop();
  void xmms2Next();
  void xmms2Prev();

private:
  QString portname;
  int buttonAdd;
  QCheckBox *toggleFx;
  QPushButton *addFxButton;


  QAction *playAction;
  QAction *pauseAction;
  QAction *stopAction;
  QAction *nextAction;
  QAction *previousAction;
};

#endif // PORTGROUPSLIDER_H
