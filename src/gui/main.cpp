#include "gui/MainWindow.h"
#include "app/SingleInstance.h"

#include <QApplication>

int main(int argc, char** argv)
{
    (void)shareaudio::enforce_single_instance();

    QApplication app(argc, argv);
    shareaudio::gui::MainWindow window;
    window.show();
    return app.exec();
}
