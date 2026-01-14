// ParthenonChain Desktop Wallet - Main Entry Point

#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName("ParthenonChain Wallet");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("ParthenonChain");
    QApplication::setOrganizationDomain("parthenonchain.org");

    // Set application icon
    app.setWindowIcon(QIcon(":/icons/parthenon-logo.svg"));

    // Create and show main window
    MainWindow window;
    window.show();

    return app.exec();
}
