#include "labelledslider.h"
#include <QtGui>

LabelledSlider::LabelledSlider(QString label, Qt::Orientation orientation, int max, int min, QWidget *parent) :
  QWidget(parent)
{


  this->slider = new QSlider();
  this->label = new QLabel();
  this->valueLabel = new QLabel();

  this->label->setText(label);
  slider->setMinimum(10*min);
  slider->setMaximum(10*max);
  slider->setOrientation(orientation);
  slider->setTracking(false);
  //slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  //this->label.setAlignment(Qt::Vertical);
  sliderLayout = new QVBoxLayout();
  if (orientation == Qt::Horizontal) {
    sliderLayout->addWidget(slider, 1);
    slider->setMinimumWidth(200);
  } else {
    slider->setMinimumHeight(200);
    sliderLayout->addWidget(slider, 1, Qt::AlignHCenter);
  }
  sliderLayout->addWidget(this->valueLabel, 0, Qt::AlignHCenter);
  sliderLayout->addWidget(this->label, 0, Qt::AlignHCenter);
  //this->valueLabel->setTextFormat(Qt::TextFormat);
  this->valueLabel->setText(tr("%1").arg(0.0, 0, 'f', 1));
  mainLayout = new QHBoxLayout(this);
  mainLayout->addLayout(sliderLayout);
  this->setLayout(mainLayout);
  //connect(slider, SIGNAL(valueChanged(int)), this->valueLabel, SLOT(setNum(int)));
  connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
  //connect(slider, SIGNAL())
}


void LabelledSlider::setValue(float value)
{
  slider->setSliderPosition((int) 10*value);
  //slider->setValue((int) 10*value);

  valueLabel->setText(tr("%1").arg(value, 0, 'f', 1));
}

void LabelledSlider::sliderValueChanged(int value)
{
  qDebug() << tr("sliderValueChanged %1 %2").arg(label->text()).arg(0.1*value);
  valueLabel->setText(tr("%1").arg((double) 0.1 * value, 0, 'f', 1));
  emit valueChanged(label->text(), 0.1f * value);
}

void LabelledSlider::updateVolume(kr_mixer_portgroup_control_rep_t *portgroup_cont)
{
  if (label->text() == tr(portgroup_cont->name)) {

    setValue(portgroup_cont->value);
  }
}

QString LabelledSlider::getLabel() {
    return label->text();
}