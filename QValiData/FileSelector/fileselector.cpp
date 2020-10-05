#include "fileselector.h"
#include "ui_fileselector.h"

FileSelector::FileSelector(QFrame *parent) : QFrame(parent),
    ui(new Ui::FileSelector)
{
    ui->setupUi(this);

    videoFile = new QString();
    dataFile = new QString();
    defaultDir = new QString();
}

FileSelector::~FileSelector()
{
    delete ui;
}

void FileSelector::on_button_videoBrowse_clicked()
{
    QString fname = QFileDialog::getOpenFileName(this, "Open Video File", *defaultDir);
    if(!fname.isEmpty()){
        delete videoFile;
        videoFile = new QString(fname);
        ui->label_videoFile->setText(*videoFile);
        emit videoFileChanged(*videoFile);
    }
    else{
    }
}

void FileSelector::on_button_dataBrowse_clicked()
{
    QString fname = QFileDialog::getOpenFileName(this, "Open Data File", *defaultDir);
    if(!fname.isEmpty()){
        delete dataFile;
        dataFile = new QString(fname);
        ui->label_dataFile->setText(*dataFile);
        emit dataFileChanged(*dataFile);
    }
    else{
    }   
}

QString FileSelector::getDataFile(){return *dataFile;}
QString FileSelector::getVideoFile(){return *videoFile;}
QString FileSelector::getDefaultDir(){return *defaultDir;}

void FileSelector::setDefaultDir(QString dir){
    delete defaultDir;
    defaultDir = new QString(dir);
}

QTableWidget *FileSelector::previewTable(){
    return (ui->tableWidget);
}

void FileSelector::setDataPreviewSize(int previewSize, int totalSize){
    ui->group_dataPreview->setTitle(QString("Data Preview (First %1 rows of %2)").arg(previewSize).arg(totalSize));
}

void FileSelector::restoreVideoFile(QString videoFile){
    delete this->videoFile;
    if(videoFile.isEmpty()){
        this->videoFile = new QString();
        ui->label_videoFile->setText(DEFAULT_STR_EMPTYFILE);
    }
    else{
        this->videoFile = new QString(videoFile);
        ui->label_videoFile->setText(videoFile);
    }
}

void FileSelector::restoreDataFile(QString dataFile){
    delete this->dataFile;
    if(dataFile.isEmpty()){
        this->dataFile = new QString();
        ui->label_dataFile->setText(DEFAULT_STR_EMPTYFILE);
        ui->tableWidget->setRowCount(0);
        ui->tableWidget->setColumnCount(0);
    }
    else{
        this->dataFile = new QString(dataFile);
        ui->label_dataFile->setText(dataFile);
    }
}
