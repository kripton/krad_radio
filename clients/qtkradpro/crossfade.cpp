#include "crossfade.h"

//Crossfade::Crossfade(QWidget *parent) :
//  LabelledSlider(parent)
//{
//}


Crossfade::Crossfade(QString portname, QString label, QWidget *parent) :
  LabelledSlider(label, Qt::Horizontal, 100, -100, parent)
{

  this->portname = portname;
  //slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  disconnect(slider, 0, 0, 0);
  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
}


void Crossfade::sliderValueChanged(int value)
{
  qDebug() << tr("crossfade sliderValueChanged %1 %2").arg(portname).arg(value);
  valueLabel->setNum((double) 0.1 * value);
  emit valueChanged(portname, (double) 0.1 * value);
}

void Crossfade::updateVolume(kr_mixer_portgroup_control_rep_t *portgroup_cont)
{
  if (portname == tr(portgroup_cont->name)) {
    setValue(portgroup_cont->value);
  } else {
    qDebug() << tr("no match <%1> <%2>").arg(portname).arg(tr(portgroup_cont->name));
  }

}
