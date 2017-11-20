#include "editsermon.h"
#include "ui_editsermon.h"

#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlError>

#include <QDebug> //may remove when debugging is finished.

EditSermon::EditSermon(QSettings *settings, QWidget *parent, QString id) :
    QDialog(parent),
    ui(new Ui::EditSermon), gsettings(settings)
{
    ui->setupUi(this);
    ui->dateEdit->setDate(QDate::currentDate());

    sermonTableModel = new QSqlTableModel(this, QSqlDatabase::database());
    sermonTableModel->setTable("sermon");
    sermonTableModel->setSort(Sermon_Date, Qt::AscendingOrder);
    sermonTableModel->select();

    sermonDataMapper = new QDataWidgetMapper(this);
    sermonDataMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
    sermonDataMapper->setModel(sermonTableModel);
    sermonDataMapper->addMapping(ui->title_lineEdit, Sermon_Title);
    sermonDataMapper->addMapping(ui->speaker_lineEdit, Sermon_Speaker);
    sermonDataMapper->addMapping(ui->location_lineEdit, Sermon_Location);
    sermonDataMapper->addMapping(ui->dateEdit, Sermon_Date);
    sermonDataMapper->addMapping(ui->description_lineEdit, Sermon_Description);

    if (id == "$#_Create_New" || sermonTableModel->rowCount() == 0) {
        //User called for new record, or else we need to create one because the table is empty.
        //Insert new row at the end of the table.
        qDebug("New record requested. Initialize table with new row.");
        int row = qMax(0, sermonTableModel->rowCount() - 1);    //added 'qMax' statement to protect against a negative row reference.
        sermonTableModel->insertRow(row);
        sermonDataMapper->setCurrentIndex(row);
    } else {
        bool foundMatch = false;
        for (int row = 0; row < sermonTableModel->rowCount(); ++row) {
            QSqlRecord record = sermonTableModel->record(row);
            if (record.value(Sermon_ID).toString() != "" && record.value(Sermon_ID).toString() == id) {
                foundMatch = true;
                sermonDataMapper->setCurrentIndex(row);
                break;
            }
            else if ((row == sermonTableModel->rowCount() - 1) && !foundMatch) {
                qDebug("No match found! Initializing table with first entry.");
                sermonDataMapper->toFirst();
            }
        }
    }
    UpdateRecordIndexLabel();

    connect(ui->pB_First, SIGNAL(clicked()), this, SLOT(toFirstSermon()));
    connect(ui->pB_Previous, SIGNAL(clicked()), this, SLOT(toPreviousSermon()));
    connect(ui->pB_Next, SIGNAL(clicked()), this, SLOT(toNextSermon()));
    connect(ui->pB_Last, SIGNAL(clicked()), this, SLOT(toLastSermon()));
}

EditSermon::~EditSermon()
{
    gsettings = NULL;
    delete sermonTableModel;
    delete sermonDataMapper;
    delete ui;
}

void EditSermon::toFirstSermon()
{
    if (!ValidateEntry())
        return; //Dialog missing some data and user opted to continue editing.

    sermonDataMapper->submit();
    sermonDataMapper->toFirst();
    UpdateRecordIndexLabel();
}

void EditSermon::toPreviousSermon()
{
    if (!ValidateEntry())
        return; //Dialog missing some data and user opted to continue editing.

    sermonDataMapper->submit();
    sermonDataMapper->toPrevious();
    UpdateRecordIndexLabel();
}

void EditSermon::toNextSermon()
{
    if (!ValidateEntry())
        return; //Dialog missing some data and user opted to continue editing.

    sermonDataMapper->submit();
    sermonDataMapper->toNext();
    UpdateRecordIndexLabel();
}

void EditSermon::toLastSermon()
{
    if (!ValidateEntry())
        return; //Dialog missing some data and user opted to continue editing.

    sermonDataMapper->submit();
    sermonDataMapper->toLast();
    UpdateRecordIndexLabel();
}

void EditSermon::on_pB_Browse_clicked()
{
    QString defaultOpenFrom = gsettings->value("paths/importFrom", "D:/").toString();
    audioFileNames = QFileDialog::getOpenFileNames(this,
          tr("Please select a sermon recording . . ."), defaultOpenFrom, tr("Audio Files (*.wav *.mp3)"));
    if (audioFileNames.isEmpty()) {
        return;
    }
    QString fileNamesList = "";
    foreach (const QString &fileName, audioFileNames) {
        QString newlineChar = "";
        if (audioFileNames.count() > 1) {
            newlineChar = "\n";
        }
        QStringList splitPath = fileName.split("/");
        QString nameOnly = splitPath.at(splitPath.count() - 1);
        fileNamesList += nameOnly + newlineChar;
    }
    ui->importAudioFiles_readOnlyEdit->setText(fileNamesList);
}


void EditSermon::on_pB_Close_clicked()
{
    if (!ValidateEntry())
        return; //Dialog missing some data and user opted to continue editing.

    QDialog::close();
}

void EditSermon::on_pB_Add_clicked()
{
    if (!ValidateEntry())
        return; //Dialog missing some data and user opted to continue editing.

    int row = sermonDataMapper->currentIndex();
    sermonDataMapper->submit();
    row++;
    sermonTableModel->insertRow(row);
    sermonDataMapper->setCurrentIndex(row);
    UpdateRecordIndexLabel();

    audioFileNames.clear(); //Re-set internal list of audio file names for the current entry.
    ui->importAudioFiles_readOnlyEdit->clear();
    ui->title_lineEdit->clear();
    ui->speaker_lineEdit->clear();
    ui->location_lineEdit->clear();
    ui->dateEdit->setDate(QDate::currentDate());
    ui->description_lineEdit->clear();
    ui->title_lineEdit->setFocus();
}

void EditSermon::on_pB_Delete_clicked()
{
    int wresult = QMessageBox::warning(this, "Delete Entry", "Are you sure you want to delete this sermon? You can choose to keep just the audio if you want to.", "Cancel", "Yes, delete everything!", "No, please keep the audio files!");
    switch (wresult){
    case 0: //Cancel
        //
        break;
    case 1: //Delete Everything
        RemoveSermon(true);
        break;
    case 2: //Save the audio, but remove the entry
        RemoveSermon(false);
        break;
    }
}

void EditSermon::UpdateRecordIndexLabel()
{
    ui->recordIndex_lbl->setText(QString("<html><head/><body><p align=center><span style= color:#ff0000;>Record %1 of %2</span></p></body></html>").arg(sermonDataMapper->currentIndex() + 1).arg(sermonTableModel->rowCount()));
    UpdateAudioFileListing();
}

void EditSermon::closeEvent(QCloseEvent *event)
{
    if (!ValidateEntry())
        event->ignore();
    else
        event->accept();
}

bool EditSermon::ValidateEntry() {
    if (audioFileNames.isEmpty() ||
            ui->title_lineEdit->text() == "" ||
            ui->speaker_lineEdit->text() == "" ||
            ui->location_lineEdit->text() == "" ||
            ui->description_lineEdit->text() == "") {
        int rvalue = QMessageBox::critical(this, "Warning!", "One or more of the fields is <b><i>empty!</i></b> If you ignore this message, this entry will be incomplete!",
                                           QMessageBox::Cancel, QMessageBox::Ignore, QMessageBox::NoButton);
        if (rvalue == QMessageBox::Cancel)
            return false;
        qDebug("User ignored warning. Continuing with partial entry.");
    }

    qDebug("Dialog closing or preparing for a different entry. Saving changes . . .");

    int currow = sermonDataMapper->currentIndex();
    sermonDataMapper->submit();
    sermonDataMapper->setCurrentIndex(currow);

    //Check to see if UUID has been generated already. If not, make a new one.
    //All read/write operations will reference the directory that matches the UUID.
    //Therefore we do not need to explicitly store file names in the database.
    if (sermonTableModel->record(sermonDataMapper->currentIndex()).field(Sermon_ID).isNull()) {
        qDebug("Null Sermon_ID detected! Checking to see if we have files to save . . .");
        if (!audioFileNames.isEmpty()) {
            //No UUID but there are files to save,
            //so create a new entry and copy in the files.
            GenerateNewEntry();
        }
    }
    return true;
}

void EditSermon::GenerateNewEntry() {
    qDebug("Creating entry and copying files . . .");
    QString destDir = gsettings->value("paths/databaseLocation", "C:/Audio Sermon Database").toString();
    QString UUID = QUuid::createUuid().toString();
    QDir objDestDir(destDir);
    objDestDir.mkdir(UUID);
    foreach (const QString &fileName, audioFileNames) {
        QStringList splitPath = fileName.split("/");
        QString nameOnly = splitPath.at(splitPath.count() - 1);
        QFile::copy(fileName, destDir + "/" + UUID + "/" + nameOnly);
    }

    //Write the new UUID back to the database.
    sermonTableModel->setData(sermonTableModel->index(sermonDataMapper->currentIndex(), Sermon_ID), UUID);
    sermonTableModel->submit();
}

void EditSermon::RemoveSermon(bool permanentlyDeleteFiles)
{
    QSqlRecord currentEntry = sermonTableModel->record(sermonDataMapper->currentIndex());

    //Delete files only if the entry is paired to some audio files
    if (!currentEntry.field(Sermon_ID).isNull()) {
        QString sourceDir = gsettings->value("paths/databaseLocation", "C:/Audio Sermon Database").toString();
        QString UUID = currentEntry.value(Sermon_ID).toString();
        QDir objSourceDir = (sourceDir + "/" + UUID);

        if (!permanentlyDeleteFiles) {
            //Back up the files to the default un-paired storage bin.
            QString storageBinDir = gsettings->value("paths/unpairedStorage", gsettings->value("paths/databaseLocation", "C:/Audio Sermon Database").toString() + "/Unpaired Audio File Storage").toString();
            QString thisBackupDirName = currentEntry.value(Sermon_Title).toString() + " - " +
                    currentEntry.value(Sermon_Speaker).toString() + " - " +
                    currentEntry.value(Sermon_Date).toString();
            QDir objStorageBinDir(storageBinDir);
            if (!objStorageBinDir.exists())
                objStorageBinDir.mkpath(storageBinDir);
            bool createDirSuccess = objStorageBinDir.mkdir(thisBackupDirName);
            QString thisStorageBinDir = storageBinDir + "/" + thisBackupDirName;

            bool copySuccess = true;
            foreach (const QString &fileName, audioFileNames) {
                QStringList splitPath = fileName.split("/");
                QString nameOnly = splitPath.at(splitPath.count() - 1);
                copySuccess = QFile::copy(fileName, thisStorageBinDir + "/" + nameOnly);
            }

            //Show messagebox indicating success or failure of backup procedure.
            if (createDirSuccess && copySuccess) {
                QMessageBox::information(this, "Backup Audio Succeeded",
                                         QString("%1 files have been successfully moved to %2")
                                         .arg(audioFileNames.count())
                                         .arg(thisStorageBinDir));
            } else {
                QMessageBox::warning(this, "Backup Audio Failed",
                    "An error occurred attempting to backup the audio files to the unpaired\nstorage bin. Since not all files were saved, the entry will not be deleted.\nPlease contact your support team for further assistance.");
                if (QDir(thisStorageBinDir).exists())
                    QDir(thisStorageBinDir).removeRecursively();
                return;
            }
        }
        //Delete the files and their UUID directory.
        if (objSourceDir.exists()) {
            if (objSourceDir.removeRecursively())
                QMessageBox::information(this, "Audio Files Removed",
                                         QString("%1 audio files and their internal parent folder have been\nsuccessfully removed from internal storage.").arg(audioFileNames.count()));
            else
                QMessageBox::warning(this, "Audio File Removal Failed",
                    "An error occurred attempting to remove the audio files from their internal storage location.\nPlease contact your support team for further assistance.");
        }
    }

    //Delete the sermon metadata from the database and refresh the table model.
    RemoveEntry();
}

void EditSermon::RemoveEntry()
{
    int row = sermonDataMapper->currentIndex();
    sermonTableModel->removeRow(row);
    sermonTableModel->select(); // Added this to pull database changes back into the table model. Previously deleted rows remained as empty entries until the next reload of the window.
    sermonDataMapper->submit();
    sermonDataMapper->setCurrentIndex(qMin(row, sermonTableModel->rowCount() - 1));
    UpdateRecordIndexLabel();
}

void EditSermon::UpdateAudioFileListing() {
    if (!sermonTableModel->record(sermonDataMapper->currentIndex()).field(Sermon_ID).isNull()) {
        //The current database field does contain a valid UUID.
        QString sourceDir = gsettings->value("paths/databaseLocation", "C:/Audio Sermon Database").toString();
        QString UUID = sermonTableModel->record(sermonDataMapper->currentIndex()).value(Sermon_ID).toString();
        QDir objSourceDir = (sourceDir + "/" + UUID);
        QString fileName = "";
        QString fileNamesList = "";
        audioFileNames = objSourceDir.entryList(QDir::Files, QDir::Name);   //Currently contains only the names.
        QString newlineChar = "";
        foreach (const QString &fileName, audioFileNames) {
            fileNamesList += newlineChar + fileName;
            if (audioFileNames.count() > 1) {
                newlineChar = "\n";
            }
        }
        //Add the complete path to the filename for direct file referencing purposes.
        //Must use a separate for loop here because the foreach loop is read-only!
        for (int i = 0; i < audioFileNames.count(); i++) {
            audioFileNames[i].prepend(objSourceDir.path() + "/");
        }
        ui->importAudioFiles_readOnlyEdit->setText(fileNamesList);
    }
}