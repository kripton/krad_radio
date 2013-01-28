#include "crossfade.h"

//Crossfade::Crossfade(QWidget *parent) :
//  LabelledSlider(parent)
//{
//}


Crossfade::Crossfade(QString portname, QString label, QWidget *parent) :
  LabelledSlider(label, parent)
{
  slider->setMaximum(100);
  slider->setMinimum(-100);
  slider->setOrientation(Qt::Horizontal);
 // slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
  this->portname = portname;
  disconnect(slider, 0, 0, 0);
  connect(slider, SIGNAL(valueChanged(int)), this->valueLabel, SLOT(setNum(int)));
  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
}


void Crossfade::sliderValueChanged(int value)
{
  qDebug() << tr("crossfade sliderValueChanged %1 %2").arg(portname).arg(value);
  emit valueChanged(portname, value);
}

void Crossfade::updateVolume(kr_mixer_portgroup_control_rep_t *portgroup_cont)
{
  if (portname == tr(portgroup_cont->name)) {
    setValue(portgroup_cont->value);
  } else {
    qDebug() << tr("no match <%1> <%2>").arg(portname).arg(tr(portgroup_cont->name));
  }

}
