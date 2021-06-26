#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("QValiData");
    persistent = new QSettings("QValiData", "persistent");
    userFile = new QSettings();
    userFileName = new QString();

    fs = new FileSelector();
    sync = new SyncView();
    track = new TrackView();

    simulators = new QList<SimulatorTab *>();
    simulatorNames = new QList<QString>();

    // Add simulator tabs here
    simulators->append(new ADXLSimView());
    simulatorNames->append("Simulate ADXL");

    simulators->append(new ActDetSimView());
    simulatorNames->append("Simulate Filter Detection");

    init();

    loadPersistent();

    connect(fs, SIGNAL(dataFileChanged(QString)), this, SLOT(gotDataFile(QString)));
    connect(fs, SIGNAL(videoFileChanged(QString)), this, SLOT(gotVideoFile(QString)));
    connect(sync, SIGNAL(syncChanged(double, double)), this, SLOT(updateSync(double, double)));
    connect(sync, SIGNAL(syncChanged(double, double)), track, SLOT(updateSync(double, double)));

    // Keep simulator tabs updated with sync and statistic window changes
    for(SimulatorTab * s: *simulators){
        connect(sync, SIGNAL(syncChanged(double, double)), s, SLOT(updateSync(double, double)));
        connect(s, SIGNAL(statChanged(double, double)), this, SLOT(updateStat(double, double)));
    }


    ui->tabWidget->addTab(fs, "Select Files");
    ui->tabWidget->addTab(sync, "Sync Video");
    ui->tabWidget->addTab(track, "Track/Annotate");

    for(int i=0; i<simulators->size(); ++i){
        ui->tabWidget->addTab(simulators->at(i), simulatorNames->at(i));
    }

    if(!videoFileValid || !dataFileValid){
        lockOtherTabs();
    }
}

void MainWindow::init(){
    cap = new VideoCapture();
    data = new TimeSeries();

    videoFileName = new QString();
    dataFileName = new QString();
    videoFileValid = false;
    dataFileValid = false;

    dataFile = new QFile();
    data = new TimeSeries();
    paths = new QList<MotionPath *>();

    track->attachPath(paths);

    for(SimulatorTab * s: *simulators){
        s->attachPath(paths);
    }

    startTime = 0;
    dataRate = 1.0;

    statStart = 0;
    statEnd = 0;
    lockOtherTabs();
}

MainWindow::~MainWindow()
{
    savePersistent();
    delete ui;
}

void MainWindow::savePersistent(){
    persistent->beginGroup("MainWindow");
    persistent->setValue("size", this->size());
    persistent->setValue("pos", this->pos());
    persistent->setValue("tab", ui->tabWidget->currentIndex());
    persistent->endGroup();
    persistent->setValue("file/user", *userFileName);
    persistent->sync();
}

void MainWindow::loadPersistent(){
    persistent->beginGroup("MainWindow");
    resize(persistent->value("size", this->size()).toSize());
    move(persistent->value("pos", this->pos()).toPoint());
    ui->tabWidget->setCurrentIndex(persistent->value("tab", 0).toInt());
    persistent->endGroup();

    userFileName = new QString(persistent->value("file/user", QString()).toString());

    if(!userFileName->isEmpty()){
        loadUserFile();
    }
    else{
        qDebug() << "User file was not found in persistent.";
    }
}

void MainWindow::loadUserFile(){
    delete userFile;
    userFile = new QSettings(*userFileName, QSettings::IniFormat);

    // Get parent directory for file
    QFileInfo fileInfo(*userFileName);
    QDir parentDirectory = fileInfo.dir();

    int numPaths = userFile->beginReadArray("motionpath");
    paths->clear();
    for(int i=0; i<numPaths; ++i){
        userFile->setArrayIndex(i);
        MotionPath *currentPath = new MotionPath();
        int pathSize = userFile->beginReadArray("points");
        for(int j=0; j<pathSize; ++j){
            userFile->setArrayIndex(j);
            currentPath->putPoint(userFile->value("frame", 0).toInt(),
                                  userFile->value("pos", QPoint(0, 0)).toPoint());
        }
        paths->append(currentPath);
        userFile->endArray();
    }
    userFile->endArray();

    track->syncPath();

    for(SimulatorTab * s: *simulators){
        s->syncPath();
    }

    userFile->beginGroup("sync");
    startTime = userFile->value("starttime", 0.0).toDouble();
    dataRate = userFile->value("datarate", 1.0).toDouble();
    userFile->endGroup();

    userFile->beginGroup("stat");
    statStart = userFile->value("statstart", 0.0).toDouble();
    statEnd = userFile->value("statend", 0.0).toDouble();
    userFile->endGroup();

    userFile->beginGroup("file");
    QString saveFileVideo = parentDirectory.absoluteFilePath(userFile->value("vidfile", QString()).toString());
    QString saveFileData = parentDirectory.absoluteFilePath(userFile->value("datafile", QString()).toString());
    userFile->endGroup();

    if(!saveFileVideo.isEmpty()){
        if(cap->isOpened()){
            cap->release();
        }
        gotVideoFile(saveFileVideo);
    }

    if(!saveFileData.isEmpty()){
        gotDataFile(saveFileData);
    }
    setWindowTitle(*userFileName + " - QValiData");
}

void MainWindow::saveUserFile(){
    if(userFile == nullptr || userFileName->isEmpty()){
        userFileName = new QString(QFileDialog::getSaveFileName(this, "Save As...", "", "Save File (*.conf)"));
        if(userFileName->isEmpty())
            return;
    }

    delete userFile;
    userFile = new QSettings(*userFileName, QSettings::IniFormat);
    // Get parent directory of settings file to calculate relative links to data and video files
    QFileInfo fileInfo(*userFileName);
    QDir parentDirectory = fileInfo.dir();
    userFile->beginGroup("motionpath");
    userFile->remove("");
    userFile->endGroup();
    userFile->beginWriteArray("motionpath");
    int numPaths = paths->size();
    for(int i=0; i<numPaths; ++i){
        userFile->setArrayIndex(i);
        MotionPath *currentpath = paths->at(i);
        userFile->beginWriteArray("points");
        int pathSize = currentpath->end - currentpath->start + 1;
        for(int j=0; j<pathSize; ++j){
            userFile->setArrayIndex(j);
            userFile->setValue("frame", j+currentpath->start);
            userFile->setValue("pos", currentpath->getPoint(j+currentpath->start));
        }
        userFile->endArray();
    }
    userFile->endArray();

    userFile->beginGroup("file");
    userFile->setValue("vidfile", parentDirectory.relativeFilePath(*videoFileName));
    userFile->setValue("datafile", parentDirectory.relativeFilePath(*dataFileName));
    userFile->endGroup();

    userFile->beginGroup("sync");
    userFile->setValue("starttime", startTime);
    userFile->setValue("datarate", dataRate);
    userFile->endGroup();

    userFile->beginGroup("stat");
    userFile->setValue("statstart", statStart);
    userFile->setValue("statend", statEnd);
    userFile->endGroup();

    userFile->sync();
}

void MainWindow::gotVideoFile(QString fname){
    delete videoFileName;
    videoFileName = new QString(fname);
    bool result = cap->open(videoFileName->toStdString());
    if(result){
        videoFileValid = true;
        fs->restoreVideoFile(*videoFileName);
        sync->attachCap(cap);
        track->attachCap(cap);
        for(SimulatorTab * s: *simulators){
            s->attachCap(cap);
        }
        if(videoFileValid && dataFileValid){
            unlockOtherTabs();
        }
    }
    else{
        QMessageBox::warning(this, "Invalid Video Format", QString("'%1'\ncannot be opened, or is not a valid video file.").arg(fname));
        lockOtherTabs();
    }
}

void MainWindow::gotDataFile(QString fname){
    delete dataFileName;
    dataFileName = new QString(fname);
    dataFile->setFileName(*dataFileName);
    delete data;
    data = new TimeSeries();
    bool openResult = data->fromCSV(dataFile, 0);
    if(openResult){
        fs->restoreDataFile(*dataFileName);


        QTableWidget *table = fs->previewTable();
        // Set table to show 10 rows or less, plus the header.
        int previewSize = qMin(data->numRows(), 10);
        table->setRowCount(previewSize+1);
        table->setColumnCount(data->numColumns());
        for(int i=0; i<data->numColumns(); i++){
            table->setItem(0, i, new QTableWidgetItem(data->columnName(i)));
        }

        for(int row=1; row<table->rowCount(); row++){
            QList<qreal> dataRow = data->rowData(row-1);
            for(int col=0; col<table->columnCount(); col++){
                table->setItem(row, col, new QTableWidgetItem(QString::number(dataRow.at(col))));
            }
        }
        fs->setDataPreviewSize(previewSize, data->numRows());

        dataFileValid = true;

        sync->attachTimeSeries(data);
        track->attachTimeSeries(data);

        for(SimulatorTab * s: *simulators){
            s->attachTimeSeries(data);
        }

        if(videoFileValid && dataFileValid){
            unlockOtherTabs();
        }
    }
    else{
        QMessageBox::warning(this, "Invalid Data Format", QString("'%1'\ncannot be opened, or is not a valid data file.").arg(fname));
        lockOtherTabs();
    }
}

// Disable all but the first tab
void MainWindow::lockOtherTabs(){
    for(int i=1; i<ui->tabWidget->count(); i++){
        ui->tabWidget->setTabEnabled(i, false);
    }
}

void MainWindow::unlockOtherTabs(){
    sync->init();
    track->init();
    for(SimulatorTab * s: *simulators){
        s->init();
    }

    sync->updateSync(startTime, dataRate);
    track->updateSync(startTime, dataRate);
    for(SimulatorTab * s: *simulators){
        s->updateSync(startTime, dataRate);
        s->updateStat(statStart, statEnd);
    }

    for(int i=1; i<ui->tabWidget->count(); i++){
        ui->tabWidget->setTabEnabled(i, true);
    }
}

void MainWindow::updateStat(double start, double end){
    statStart = start;
    statEnd = end;

    for(SimulatorTab * s: *simulators){
        s->updateStat(statStart, statEnd);
    }
}

void MainWindow::updateSync(double start, double rate){
    startTime = start;
    dataRate = rate;
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if(index == ui->tabWidget->indexOf(sync)){
        sync->syncCap();
    }
    else if(index == ui->tabWidget->indexOf(track)){
        track->syncCap();
        track->syncPath();
    }
    else{
        for(SimulatorTab * s: *simulators){
            if(index == ui->tabWidget->indexOf(s)){
                s->syncCap();
                s->syncPath();
            }
        }
    }
}

void MainWindow::on_actionNew_triggered()
{
    userFile = new QSettings();
    userFileName = new QString();
    fs->restoreDataFile("");
    fs->restoreVideoFile("");
    setWindowTitle("QValiData");
    init();
}

void MainWindow::on_actionOpen_triggered()
{
    QString *userFileNameTemp = new QString(QFileDialog::getOpenFileName(this, "Open...", "", "Save File (*.conf)"));
    if(!userFileNameTemp->isEmpty()){
        delete userFileName;
        userFileName = new QString(*userFileNameTemp);
    }
    loadUserFile();
}

void MainWindow::on_actionSave_triggered()
{
    saveUserFile();
}

void MainWindow::on_pushButton_clicked()
{
    saveUserFile();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "", "QValiData 1.1.1\nDeveloped by Jiawei Chen\n\
Special thanks to:\n     Geoffrey Brown\n     Adam Fudickar\n     David Crandall\n\
This material is based upon work supported by the National Science Foundation under Grant No. 1644717.\n\
Any opinions, findings, and conclusions or recommendations expressed in this material are those of the authors \
and do not necessarily reflect the views of the National Science Foundation.");
}
