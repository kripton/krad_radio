#ifndef CROSSFADE_H
#define CROSSFADE_H

#include <QWidget>
#include "labelledslider.h"

class Crossfade : public LabelledSlider
{
  Q_OBJECT
public:
  explicit Crossfade(QWidget *parent = 0);
  Crossfade(QString portname, QString label, QWidget *parent = 0);
signals:
  void valueChanged(QString name, float value);

public slots:
//  void setValue (int value);
  void sliderValueChanged(int value);
  void updateVolume(kr_mixer_portgroup_control_rep_t *);


private:
  QString portname;

//  QSlider *slider;

};
#endif // CROSSFADE_H
