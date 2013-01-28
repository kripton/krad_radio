#ifndef EQSLIDER_H
#define EQSLIDER_H
#include <QWidget>
#include "labelledslider.h"
class EqSlider : public LabelledSlider
{
  Q_OBJECT
public:
  explicit EqSlider(QWidget *parent = 0);
  EqSlider(int bandId, QWidget *parent = 0);
  EqSlider(int bandId, float freq, QWidget *parent = 0);
signals:
  void bandAdded(int bandId, float freq);
  void valueChanged(int bandId, int value);
public slots:
  void sliderValueChanged(int value);
private:
  int bandId;
  float freq;
};

#endif // EQSLIDER_H
