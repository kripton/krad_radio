#ifndef EQ_H
#define EQ_H

#include <QWidget>
#include "eqslider.h"

class EqWidget : public QWidget
{
  Q_OBJECT
public:
  explicit EqWidget(QString portname, QWidget *parent = 0);

signals:
  void bandAdded(QString portname, int bandId, float freq);
  void bandDbChanged(QString portname, int bandId, int freq);
  void eqDestroyed(QString portname);
public slots:
  void addBand(float freq);
  void bandDbChanged(int bandId, int freq);
  void destroyed();
  //void eqUpdated()
private:

  QHBoxLayout *eqLayout;
  QString portname;
  int fxId;
  QList<float> freqs;
};

#endif // EQ_H
