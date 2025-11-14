#include "ProblemGenerator.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ProblemGenerator window;
    //window.setWindowIcon(QIcon(":/ProblemGenerator.ico"));
    window.show();
    return app.exec();
}