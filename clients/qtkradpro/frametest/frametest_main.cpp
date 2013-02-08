/*#include "frametest_mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}*/


//#########################################
#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QTextEdit>
#include <QGraphicsView>
#include <QGraphicsScene>

#include "resizableframe.h"

int main (int argc,char* argv[]){
    QApplication app(argc,argv);

    ResizableFrame *f1=new ResizableFrame();
    f1->setGeometry(50,50,100,100);
    ResizableFrame *f2=new ResizableFrame();
    f2->setGeometry(200,200,100,100);

    QGraphicsScene scene(0,0,800,600);

    QGraphicsProxyWidget * c1=scene.addWidget(f1);
    f1->setProxy(c1);
    QGraphicsProxyWidget * c2=scene.addWidget(f2);
    f2->setProxy(c2);

    //c1->rotate(45);
    //c2->rotate(20);

    QGraphicsView view(&scene);
    view.show();

    return app.exec();
}
//#########################################
