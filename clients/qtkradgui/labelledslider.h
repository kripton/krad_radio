#ifndef LABELLEDSLIDER_H
#define LABELLEDSLIDER_H

#include <QObject>
#include <QSlider>
#include <QtGui>
#include <QLabel>

#include "kr_client.h"

class LabelledSlider : public QWidget
{
  Q_OBJECT
public:
  explicit LabelledSlider(QWidget *parent = 0);
  LabelledSlider(QString label, Qt::Orientation orientation, int max, int min, QWidget *parent = 0);
  QString getLabel();

signals:
  void valueChanged(QString name, float value);
public slots:
  void setValue (float value);
  void sliderValueChanged(int value);
  void updateVolume(kr_mixer_portgroup_control_rep_t *);
private:

protected:
  QVBoxLayout *layout;
  QLabel *label;
  QSlider *slider;
  QLabel *valueLabel;
};

#endif
