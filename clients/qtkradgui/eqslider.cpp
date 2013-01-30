#include "eqslider.h"

//EqSlider::EqSlider(QWidget *parent) :
//  LabelledSlider(parent)
//{
//}

EqSlider::EqSlider(int bandId, QWidget *parent) :
  LabelledSlider(tr("%1").arg(bandId), Qt::Vertical, 32, -32, parent)
{

  this->bandId = bandId;
  disconnect(slider, 0, 0, 0);
  connect(slider, SIGNAL(valueChanged(int)), this->valueLabel, SLOT(setNum(int)));
  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
}

EqSlider::EqSlider(int bandId, float freq, QWidget *parent) :
  LabelledSlider(tr("%1 hz").arg(freq), Qt::Vertical, 20, -20, parent)
{

    this->bandId = bandId;
    disconnect(slider, 0, 0, 0);

    connect(slider, SIGNAL(valueChanged(int)), this->valueLabel, SLOT(setNum(int)));
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
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
