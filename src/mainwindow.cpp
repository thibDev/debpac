#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filesignatureinfo.hpp"
#include "treeview.h"
#include "tabwidget.h"
#include <QListView>
#include <QGridLayout>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Debian package helper");
    setWindowIcon(QIcon("://icon/package.png"));
    gLayout = new QGridLayout(ui->centralWidget);
    ui->centralWidget->setLayout(gLayout);

    splitter = new QSplitter(Qt::Horizontal, this);
    tabWidget = new TabWidget(this);
    treeView = new TreeView(this);

    splitter->addWidget(treeView);
    splitter->addWidget(tabWidget);
    gLayout->addWidget(splitter, 0, 0);

    splitter->setStretchFactor(1, 2);
    resize(700, 400);
}

MainWindow::~MainWindow()
{
    delete treeView;
    delete tabWidget;
    delete splitter;
    delete gLayout;
    delete ui;
}
