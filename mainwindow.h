#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include <QMainWindow>
#include <QTableWidgetItem>
#include "nal_parse.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    std::string video_info_to_string(videoinfo_t *videoInfo);
    void add_nalu(NALU_t *nal);

private slots:
    void on_actionOpen_File_triggered();
    void on_actionAbout_triggered();

    void on_tableWidget_itemClicked(QTableWidgetItem *item);

private:
    CNalParser m_parser;
    std::vector<NALU_t> m_vctNalu;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
