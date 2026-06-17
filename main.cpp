#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    QApplication::setApplicationName("ClineLikeChat");

    MainWindow w;
    w.show();
    return app.exec();
}
