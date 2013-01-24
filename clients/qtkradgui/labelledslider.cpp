#include "labelledslider.h"
#include <QtGui>

LabelledSlider::LabelledSlider(QString label, QWidget *parent) :
  QWidget(parent)
{

  QSizePolicy size;
  size.setHorizontalPolicy(QSizePolicy::Expanding);
  size.setVerticalPolicy(QSizePolicy::Expanding);
  size.setVerticalStretch(1);
  this->slider = new QSlider();
  this->label = new QLabel();

  this->label->setText(label);
  slider->setMaximum(100);
  //slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  //this->label.setAlignment(Qt::Vertical);
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(slider, 1, Qt::AlignHCenter);
  layout->addWidget(this->label, 0,  Qt::AlignHCenter);

  this->setLayout(layout);

  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
}

void LabelledSlider::setValue(int value)
{
  slider->setValue(value);
}

void LabelledSlider::sliderValueChanged(int value)
{
  qDebug() << tr("sliderValueChanged %1 %2").arg(label->text()).arg(value);
  emit valueChanged(label->text(), value);
}

void LabelledSlider::updateVolume(kr_mixer_portgroup_control_rep_t *portgroup_cont)
{
  if (label->text() == tr(portgroup_cont->name)) {
    setValue(portgroup_cont->value);
  } else {
    qDebug() << tr("no match <%1> <%2>").arg(label->text()).arg(tr(portgroup_cont->name));
  }

}
