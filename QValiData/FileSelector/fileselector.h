#ifndef FILESELECTOR_H
#define FILESELECTOR_H

#include <QFrame>
#include <QTableWidget>
#include <QFileDialog>

#define DEFAULT_STR_EMPTYFILE "[No File Selected]"

namespace Ui{
class FileSelector;
}

class FileSelector : public QFrame
{
    Q_OBJECT
public:
    explicit FileSelector(QFrame *parent = nullptr);
    ~FileSelector();

    QString getVideoFile();
    QString getDataFile();
    QString getDefaultDir();
    void setDefaultDir(QString dir);
    QTableWidget * previewTable();
    void setDataPreviewSize(int previewSize, int totalSize);

private:
    Ui::FileSelector *ui;
    // Path to video file
    QString *videoFile;
    // Path to data file
    QString *dataFile;
    // Previously-opened directory, if there is one
    QString *defaultDir;

signals:
    void videoFileChanged(QString fname);
    void dataFileChanged(QString fname);

public slots:
    void restoreVideoFile(QString videoFile);
    void restoreDataFile(QString dataFile);
private slots:
    void on_button_videoBrowse_clicked();
    void on_button_dataBrowse_clicked();

};

#endif // FILESELECTOR_H
