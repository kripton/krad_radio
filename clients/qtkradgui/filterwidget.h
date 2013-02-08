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
  void hiPassFilterUpdated(QString portname, QString control, float value);
  void loPassFilterUpdated(QString portname, QString control, float value);
public slots:
  void hipassUpdated(QString name, float value);
  void lopassUpdated(QString name, float value);
private:
  QVBoxLayout *layout;
  QString portname;
  int fxId;
  LabelledSlider *lopassSlider;
  LabelledSlider *hipassSlider;
};

#endif // FILTERWIDGET_H
