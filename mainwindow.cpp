#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingswindow.h"
#include "editsermon.h"
#include "findsermon.h"
#include "publishsermon.h"
#include "databasesupport.h"

#define ABOUTTEXT \
    "<i><b>Message Librarian</b> © 2016 - 2019 by Stanley B. Gehman.</i><p>"\
    "This software is intended to assist the organizational efforts of those "\
    "responsible to maintain audio sermon libraries. It is free of charge; however, "\
    "if you wish to contribute to the project either through code or by donations, "\
    "we do welcome your input. Due to the nature of free software, we do not provide "\
    "any warranty, expressed or implied.<p>Tech support may be somewhat limited, but "\
    "we encourage you to report bugs and request new features. For more details, "\
    "please see the README.md file included with the "\
    "<a href="https://github.com/Stanley-G/message-librarian">source code on GitHub</a>. "\
    "We look forward to hearing from you.<p>Contact: <b>Stanley Gehman</b> Email: "\
    "<b><u>sg.tla@emypeople.net</u></b> Phone: <b>(217) 254 - 4403</b>"\

#define NOTIMPLEMENTEDTEXT \
    "We're sorry, this feature is not available in this release. "\
    "Please stay tuned for available updates!"\

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    globalSettings = new QSettings("TrueLife Tracks", "Message Librarian", this);
    findwin = NULL;

    InitTableModelAndView();
}

void MainWindow::InitTableModelAndView()
{
    sermonTableModel = new QSqlTableModel(this, QSqlDatabase::database());
    sermonTableModel->setTable(DatabaseSupport::GetCompatibleDBTableName());
    sermonTableModel->setSort(Sermon_Date, Qt::AscendingOrder);
    sermonTableModel->select();

    sortFilterSermonModel = new SermonSortFilterProxyModel();   //Invokes our custom sermon search-and-sort engine
    sortFilterSermonModel->setSourceModel(sermonTableModel);

    sermonTableModel->setHeaderData(Sermon_ID, Qt::Horizontal, "Audio Binding");
    sermonTableModel->setHeaderData(Sermon_Title, Qt::Horizontal, "Title");
    sermonTableModel->setHeaderData(Sermon_Speaker, Qt::Horizontal, "Speaker");
    sermonTableModel->setHeaderData(Sermon_Location, Qt::Horizontal, "Location");
    sermonTableModel->setHeaderData(Sermon_Date, Qt::Horizontal, "Date");
    sermonTableModel->setHeaderData(Sermon_Description, Qt::Horizontal, "Description");
    sermonTableModel->setHeaderData(Sermon_Transcription, Qt::Horizontal, "Transcription");

    ui->mainSermonTableView->setModel(sortFilterSermonModel);
    ui->mainSermonTableView->setSortingEnabled(true); //Turn sort-by-header-click on.
    ui->mainSermonTableView->horizontalHeader()->setSortIndicator(Sermon_Date, Qt::AscendingOrder); //Specifies the default sort order and column for the table view.
    ui->mainSermonTableView->horizontalHeader()->moveSection(Sermon_ID, Sermon_Description);    //Awsome code!! moves columns around in the table view without changing the order in the sql table itself!
    ui->mainSermonTableView->horizontalHeader()->setSectionResizeMode(Sermon_Title, QHeaderView::Stretch);
    ui->mainSermonTableView->resizeColumnsToContents();
    ui->mainSermonTableView->setColumnWidth(Sermon_ID, 130);
    ui->mainSermonTableView->setColumnWidth(Sermon_Transcription, 130);
    ui->mainSermonTableView->setItemDelegateForColumn(Sermon_ID, new StatusIndicatorDelegate);  //Invokes our custom state indicator icons for certain data types in our table.
    ui->mainSermonTableView->setItemDelegateForColumn(Sermon_Transcription, new StatusIndicatorDelegate);   //Same here.

    ui->mainSermonTableView->selectRow(globalSettings->value("metadata/lastActiveSermon", "0").toInt());
    on_mainSermonTableView_clicked(sermonTableModel->index(globalSettings->value("metadata/lastActiveSermon", "0").toInt(), 1)); //It is not necessary to know which column, as that is specified later.

    setCentralWidget(ui->mainSermonTableView);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete globalSettings;
    delete sermonTableModel;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //Save current sermon selection from main table.
    globalSettings->setValue("metadata/lastActiveSermon", ui->mainSermonTableView->currentIndex().row());
    event->accept();
}

//This is a public setter
void MainWindow::SetCurrentModelIndex(QPersistentModelIndex *index)
{
    currentModelIndex = index;
    ui->mainSermonTableView->selectRow(currentModelIndex->row());
    on_mainSermonTableView_clicked(static_cast <QModelIndex> (*index)); //impressive cast operation!
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "Audio Sermon Organizer", ABOUTTEXT);
}

void MainWindow::on_actionExit_triggered()
{
    // This code runs AFTER the main window is destroyed. All saving of data must be done in the reimplemented 'closeEvent'!
    QCoreApplication::quit();
}

void MainWindow::on_actionEdit_triggered()
{
    //Here we set the ID of the currently selected sermon in MainWindow
    currentModelIndex = new QPersistentModelIndex(ui->mainSermonTableView->currentIndex());
    ui->mainSermonTableView->selectionModel()->reset();  //Added this to force custom delegates to repaint when they lose focus.
    EditSermon newwin(globalSettings, sermonTableModel, this, "", currentModelIndex);
    newwin.exec();
}

void MainWindow::on_actionPreferences_triggered()
{
    SettingsWindow swin(globalSettings, this);
    swin.exec();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionNew_triggered()
{
    ui->mainSermonTableView->selectionModel()->reset();  //Added this to force custom delegates to repaint when they lose focus.
    EditSermon newwin(globalSettings, sermonTableModel, this, "$#_Create_New"); //Invoke Edit Sermon ui with new blank sermon entry at the end.
    newwin.exec();
}

void MainWindow::on_actionOpen_triggered()
{
    QMessageBox::critical(this, "Not Implemented Yet", NOTIMPLEMENTEDTEXT);
}

void MainWindow::on_actionSearch_triggered()
{
    if (findwin == NULL) {  //This test avoids stacking multiple copies of the dialog on top of each other
        findwin = new FindSermon(sortFilterSermonModel, this);
    }

    ui->mainSermonTableView->selectionModel()->reset();  //Added this to force custom delegates to repaint when they lose focus.
    findwin->show();
    findwin->raise();
    findwin->activateWindow();
}

void MainWindow::on_actionPublish_triggered()
{
    /* Development direction:
     *  Have a warning dialog pop up if no audio is bound to the entry. Give option to edit sermon where they could add files.
     *  If audio is available, display a dialog listing the files, with total playback time (to ensure that they will fit on CD.)
     *  - OR - do we want to do this check before we get this far, i.e. by disabling this action in MenuBar, etc. <- Chosen solution, for the time being
     */

    // Deprecate unfinished feature for alpha release to VIPs.
    /* ui->mainSermonTableView->selectionModel()->reset();  //Added this to force custom delegates to repaint when they lose focus.
     * PublishSermon pubwin(globalSettings, sermonTableModel, currentModelIndex, this);
     * pubwin.exec();
     */
    QMessageBox::critical(this, "Not Implemented Yet", NOTIMPLEMENTEDTEXT);
}

void MainWindow::on_mainSermonTableView_doubleClicked(const QModelIndex &index)
{
    //This will eventually OPEN the sermon, not EDIT it.
    currentModelIndex = new QPersistentModelIndex(index);
    ui->mainSermonTableView->selectionModel()->reset();  //Added this to force custom delegates to repaint when they lose focus.
    EditSermon newwin(globalSettings, sermonTableModel, this, "", currentModelIndex); //Invoke Edit Sermon ui with current selected row from MainWindow as the current Sermon.
    newwin.exec();
}

void MainWindow::on_mainSermonTableView_clicked(const QModelIndex &index)
{
    /*
     * enable/disable certain actions based on available data.
     */

    // Disable Publishing if no audio is available.
    if (sermonTableModel->record(index.row()).field(Sermon_ID).isNull())
       ui->actionPublish->setEnabled(false);
    else
        ui->actionPublish->setEnabled(true);
}
