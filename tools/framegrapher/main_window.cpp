#include "main_window.hpp"
#include "./ui_main_window.h"

#include <QLineSeries>

main_window::main_window (QWidget *parent)
    : QMainWindow (parent)
    , ui (new Ui::main_window)
{
    ui->setupUi (this);

    QLineSeries *series = new QLineSeries ();
    series->append (0, 6);
    series->append (2, 4);

    ui->chartView->chart ()->addSeries (series);
    ui->chartView->chart ()->createDefaultAxes ();
}

main_window::~main_window ()
{
    delete ui;
}

