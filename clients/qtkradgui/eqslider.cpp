#include "eqslider.h"

//EqSlider::EqSlider(QWidget *parent) :
//  LabelledSlider(parent)
//{
//}

EqSlider::EqSlider(int bandId, QWidget *parent) :
  LabelledSlider(tr("%1").arg(bandId), parent)
{
  slider->setMaximum(32);
  slider->setMinimum(-32);

  this->bandId = bandId;
  disconnect(slider, 0, 0, 0);
  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
}

EqSlider::EqSlider(int bandId, float freq, QWidget *parent) :
  EqSlider(bandId, parent)
{
  this->freq = freq;
}


void EqSlider::sliderValueChanged(int value)
{
  qDebug() << tr("EqSlider sliderValueChanged %1 %2").arg(bandId).arg(value);
  emit valueChanged(bandId, value);
}

//void EqSlider::updateVolume(kr_mixer_portgroup_control_rep_t *portgroup_cont)
//{
//  if (portname == tr(portgroup_cont->name)) {
//    setValue(portgroup_cont->value);
//  } else {
//    qDebug() << tr("no match <%1> <%2>").arg(portname).arg(tr(portgroup_cont->name));
//  }

//}
