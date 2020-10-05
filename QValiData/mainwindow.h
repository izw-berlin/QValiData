#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <opencv2/opencv.hpp>
#include "fileselector.h"
#include "syncview.h"
#include "timeseries.h"
#include "trackview.h"
#include "adxlsimview.h"
#include "actdetsimview.h"
#include "simulatortab.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadPersistent();
    void savePersistent();
    void loadUserFile();
    void saveUserFile();
    void init();

private:
    Ui::MainWindow *ui;
    // persistent keeps track of UI placement, sizing, and last opened file.
    QSettings *persistent;
    // userFile is the specific project file opened.
    QSettings *userFile;
    QString *userFileName;

    FileSelector *fs;

    QString *videoFileName;
    QString *dataFileName;

    bool videoFileValid;
    bool dataFileValid;

    VideoCapture *cap;
    TimeSeries *data;
    QFile *dataFile;

    SyncView *sync;
    double startTime;
    double dataRate;

    TrackView *track;
    QList<MotionPath *> *paths;

    QList<SimulatorTab *> *simulators;
    QList<QString> *simulatorNames;
    double statStart;
    double statEnd;

    void lockOtherTabs();
    void unlockOtherTabs();

private slots:
    void gotVideoFile(QString fname);
    void gotDataFile(QString fname);
    void updateSync(double start, double rate);
    void updateStat(double start, double end);
    void on_tabWidget_currentChanged(int index);
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_pushButton_clicked();
    void on_actionAbout_triggered();
};

#endif // MAINWINDOW_H
