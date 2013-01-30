#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <QWidget>
#include <QSlider>
#include "labelledslider.h"

class FilterWidget : public QWidget
{
  Q_OBJECT
public:
  explicit FilterWidget(QString portname, QWidget *parent = 0);
  
signals:
  void filterUpdated(QString portname, QString control, int value);
public slots:
  void hipassUpdated(QString name, int value);
  void lopassUpdated(QString name, int value);
private:
  QVBoxLayout *layout;
  QString portname;
  int fxId;
  LabelledSlider *lopassSlider;
  LabelledSlider *hipassSlider;
};

#endif // FILTERWIDGET_H
