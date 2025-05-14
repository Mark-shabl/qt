#include "databaseadmin.h"
#include <QApplication>
#include <QTableView>
#include <QTextEdit>
#include <QStatusBar>
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QSqlError>
#include <QSqlQuery>
#include <QHeaderView>
#include <QClipboard>
#include <QFile>
#include <QTextStream>
#include <QCloseEvent>
#include <QStandardItemModel>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QDebug>
#include <QFormLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialogButtonBox>

DatabaseAdmin::DatabaseAdmin(QWidget *parent)
    : QMainWindow(parent),
    sqlModel(new QSqlQueryModel(this)),
    csvModel(new QStandardItemModel(this)),
    settings(new QSettings("DatabaseAdmin", "QtDBAdmin", this))
{
    setupUI();
    loadSettings();
}

DatabaseAdmin::~DatabaseAdmin()
{
    saveSettings();
    if (QSqlDatabase::database().isOpen()) {
        QSqlDatabase::database().close();
    }
}

void DatabaseAdmin::setupUI()
{
    // Main widgets
    tableView = new QTableView(this);
    queryEditor = new QTextEdit(this);
    statusBar = new QStatusBar(this);

    // Setup models
    tableView->setModel(sqlModel);

    // Query dock
    queryDock = new QDockWidget(tr("SQL Query"), this);
    queryDock->setWidget(queryEditor);
    addDockWidget(Qt::BottomDockWidgetArea, queryDock);

    // Main window setup
    setCentralWidget(tableView);
    setStatusBar(statusBar);
    setWindowTitle(tr("Database Administrator"));
    resize(1000, 700);

    // Table view settings
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setEditTriggers(QAbstractItemView::DoubleClicked);
    tableView->setSortingEnabled(true);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView->verticalHeader()->setVisible(false);

    // Create menus and toolbars
    createMenus();
    createToolBars();
    createStatusBar();

    // Initial state
    disconnectAction->setEnabled(false);
    refreshAction->setEnabled(false);
}

void DatabaseAdmin::createMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    connectAction = fileMenu->addAction(tr("&Connect..."), this, &DatabaseAdmin::connectToDatabase);
    disconnectAction = fileMenu->addAction(tr("&Disconnect"), this, &DatabaseAdmin::disconnectFromDatabase);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Database &Info"), this, &DatabaseAdmin::showDatabaseInfo);
    fileMenu->addSeparator();
    exportAction = fileMenu->addAction(tr("&Export to CSV..."), this, &DatabaseAdmin::exportToCSV);
    importAction = fileMenu->addAction(tr("&Import from CSV..."), this, &DatabaseAdmin::importFromCSV);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Preferences..."), this, &DatabaseAdmin::showPreferences);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), qApp, &QApplication::closeAllWindows);

    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    refreshAction = editMenu->addAction(tr("&Refresh"), this, &DatabaseAdmin::refreshData);
    refreshAction->setShortcut(QKeySequence::Refresh);

    editMenu->addSeparator();

    copyAction = editMenu->addAction(tr("&Copy"), this, &DatabaseAdmin::copyData);
    copyAction->setShortcut(QKeySequence::Copy);

    pasteAction = editMenu->addAction(tr("&Paste"), this, &DatabaseAdmin::pasteData);
    pasteAction->setShortcut(QKeySequence::Paste);

    deleteAction = editMenu->addAction(tr("&Delete Rows"), this, &DatabaseAdmin::deleteSelectedRows);
    insertAction = editMenu->addAction(tr("&Insert Row"), this, &DatabaseAdmin::insertRow);

    // Table menu
    QMenu *tableMenu = menuBar()->addMenu(tr("&Table"));
    showTablesAction = tableMenu->addAction(tr("&Show Tables"), this, &DatabaseAdmin::showTables);
    tableMenu->addAction(tr("Show &Structure"), this, &DatabaseAdmin::showTableStructure);
    tableMenu->addSeparator();
    tableMenu->addAction(tr("&Create Table..."), this, &DatabaseAdmin::createTable);
    tableMenu->addAction(tr("&Drop Table..."), this, &DatabaseAdmin::dropTable);
    tableMenu->addAction(tr("&Rename Table..."), this, &DatabaseAdmin::renameTable);
    tableMenu->addAction(tr("&Truncate Table..."), this, &DatabaseAdmin::truncateTable);

    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    filterAction = viewMenu->addAction(tr("&Filter Data..."), this, &DatabaseAdmin::filterData);
    sortAction = viewMenu->addAction(tr("&Sort Data..."), this, &DatabaseAdmin::sortData);
    resetAction = viewMenu->addAction(tr("&Reset View"), this, &DatabaseAdmin::resetView);

    // Query menu
    QMenu *queryMenu = menuBar()->addMenu(tr("&Query"));
    executeAction = queryMenu->addAction(tr("&Execute"), this, &DatabaseAdmin::executeQuery);
    executeAction->setShortcut(Qt::Key_F5);  // Устанавливаем горячую клавишу отдельно

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &DatabaseAdmin::about);
}

void DatabaseAdmin::createToolBars()
{
    // Database toolbar
    QToolBar *dbToolBar = addToolBar(tr("Database"));
    dbToolBar->addAction(connectAction);
    dbToolBar->addAction(disconnectAction);
    dbToolBar->addSeparator();
    dbToolBar->addAction(refreshAction);

    // Table toolbar
    QToolBar *tableToolBar = addToolBar(tr("Table"));
    tableToolBar->addAction(showTablesAction);
    tableToolBar->addSeparator();
    tableToolBar->addAction(exportAction);
    tableToolBar->addAction(importAction);

    // Data toolbar
    QToolBar *dataToolBar = addToolBar(tr("Data"));
    dataToolBar->addAction(copyAction);
    dataToolBar->addAction(pasteAction);
    dataToolBar->addAction(deleteAction);
    dataToolBar->addAction(insertAction);

    // Query toolbar
    QToolBar *queryToolBar = addToolBar(tr("Query"));
    queryToolBar->addAction(executeAction);
}

void DatabaseAdmin::createStatusBar()
{
    statusBar->showMessage(tr("Ready"));
}

void DatabaseAdmin::loadSettings()
{
    settings->beginGroup("MainWindow");
    restoreGeometry(settings->value("geometry").toByteArray());
    restoreState(settings->value("windowState").toByteArray());
    settings->endGroup();

    settings->beginGroup("Preferences");
    lastDir = settings->value("lastDir", QDir::homePath()).toString();
    autoRefresh = settings->value("autoRefresh", false).toBool();
    refreshInterval = settings->value("refreshInterval", 30).toInt();
    lastDatabase = settings->value("lastDatabase").toString();
    settings->endGroup();

    if (!lastDatabase.isEmpty()) {
        queryEditor->setPlainText(QString("USE %1;").arg(lastDatabase));
    }
}

void DatabaseAdmin::saveSettings()
{
    settings->beginGroup("MainWindow");
    settings->setValue("geometry", saveGeometry());
    settings->setValue("windowState", saveState());
    settings->endGroup();

    settings->beginGroup("Preferences");
    settings->setValue("lastDir", lastDir);
    settings->setValue("autoRefresh", autoRefresh);
    settings->setValue("refreshInterval", refreshInterval);
    settings->setValue("lastDatabase", lastDatabase);
    settings->endGroup();
}

void DatabaseAdmin::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

// Database operations
void DatabaseAdmin::connectToDatabase()
{
    QString host = QInputDialog::getText(this, tr("Connect to Database"),
                                        tr("Host:"), QLineEdit::Normal, "localhost");
    if (host.isEmpty()) return;

    int port = QInputDialog::getInt(this, tr("Connect to Database"),
                                    tr("Port:"), 3306, 1, 65535);

    QString database = QInputDialog::getText(this, tr("Connect to Database"),
                                            tr("Database name:"));
    if (database.isEmpty()) return;

    QString username = QInputDialog::getText(this, tr("Connect to Database"),
                                            tr("Username:"), QLineEdit::Normal, "root");
    if (username.isEmpty()) return;

    QString password = QInputDialog::getText(this, tr("Connect to Database"),
                                            tr("Password:"), QLineEdit::Password);

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(database);
    db.setUserName(username);
    db.setPassword(password);

    if (!db.open()) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to connect to database:\n%1").arg(db.lastError().text()));
        return;
    }

    lastDatabase = database;
    statusBar->showMessage(tr("Connected to %1@%2").arg(database).arg(host), 3000);

    // Enable/disable actions
    disconnectAction->setEnabled(true);
    refreshAction->setEnabled(true);
    connectAction->setEnabled(false);

    // Show tables
    showTables();
}

void DatabaseAdmin::disconnectFromDatabase()
{
    if (!confirmAction(tr("Are you sure you want to disconnect from the database?"))) {
        return;
    }

    QSqlDatabase::database().close();
    sqlModel->clear();
    statusBar->showMessage(tr("Disconnected"), 3000);

    // Enable/disable actions
    disconnectAction->setEnabled(false);
    refreshAction->setEnabled(false);
    connectAction->setEnabled(true);
}

void DatabaseAdmin::showDatabaseInfo()
{
    if (!checkDatabaseConnection()) return;

    QSqlQuery query;
    QString info;

    // Server version
    if (query.exec("SELECT VERSION()")) {
        query.next();
        info += tr("MySQL Version: %1\n").arg(query.value(0).toString());
    }

    // Current database
    if (query.exec("SELECT DATABASE()")) {
        query.next();
        info += tr("Current Database: %1\n").arg(query.value(0).toString());
    }

    // User
    if (query.exec("SELECT USER()")) {
        query.next();
        info += tr("Current User: %1\n").arg(query.value(0).toString());
    }

    // Character set
    if (query.exec("SHOW VARIABLES LIKE 'character_set_database'")) {
        query.next();
        info += tr("Character Set: %1\n").arg(query.value(1).toString());
    }

    // Collation
    if (query.exec("SHOW VARIABLES LIKE 'collation_database'")) {
        query.next();
        info += tr("Collation: %1\n").arg(query.value(1).toString());
    }

    QMessageBox::information(this, tr("Database Information"), info);
}

void DatabaseAdmin::refreshData()
{
    if (!checkDatabaseConnection()) return;

    QString currentTable = currentTableName();
    if (!currentTable.isEmpty()) {
        executeAndShowQuery(QString("SELECT * FROM %1").arg(currentTable));
        statusBar->showMessage(tr("Data refreshed"), 2000);
    } else {
        showTables();
    }
}

// Table operations
void DatabaseAdmin::showTables()
{
    if (!checkDatabaseConnection()) return;

    executeAndShowQuery("SHOW TABLES");
    statusBar->showMessage(tr("Tables loaded"), 2000);
}

void DatabaseAdmin::showTableStructure()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    executeAndShowQuery(QString("DESCRIBE %1").arg(tableName));
    statusBar->showMessage(tr("Structure of %1").arg(tableName), 2000);
}

void DatabaseAdmin::createTable()
{
    if (!checkDatabaseConnection()) return;

    bool ok;
    QString tableName = QInputDialog::getText(this, tr("Create Table"),
                                            tr("Table name:"), QLineEdit::Normal, "", &ok);
    if (!ok || tableName.isEmpty()) return;

    QString columns = QInputDialog::getMultiLineText(this, tr("Create Table"),
                                                    tr("Column definitions (one per line):"),
                                                    "id INT PRIMARY KEY AUTO_INCREMENT", &ok);
    if (!ok || columns.isEmpty()) return;

    QStringList columnDefs = columns.split('\n', Qt::SkipEmptyParts);
    QString queryStr = QString("CREATE TABLE %1 (%2)").arg(tableName).arg(columnDefs.join(", "));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        showError(tr("Create Table Failed"), query.lastError());
        return;
    }

    statusBar->showMessage(tr("Table %1 created").arg(tableName), 3000);
    showTables();
}

void DatabaseAdmin::dropTable()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    if (!confirmAction(tr("Are you sure you want to drop table %1?").arg(tableName))) {
        return;
    }

    QSqlQuery query;
    if (!query.exec(QString("DROP TABLE %1").arg(tableName))) {
        showError(tr("Drop Table Failed"), query.lastError());
        return;
    }

    statusBar->showMessage(tr("Table %1 dropped").arg(tableName), 3000);
    showTables();
}

void DatabaseAdmin::renameTable()
{
    if (!checkDatabaseConnection()) return;

    QString oldName = currentTableName();
    if (oldName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Table"),
                                            tr("New table name:"), QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.isEmpty() || newName == oldName) return;

    QSqlQuery query;
    if (!query.exec(QString("RENAME TABLE %1 TO %2").arg(oldName).arg(newName))) {
        showError(tr("Rename Table Failed"), query.lastError());
        return;
    }

    statusBar->showMessage(tr("Table renamed from %1 to %2").arg(oldName).arg(newName), 3000);
    showTables();
}

void DatabaseAdmin::truncateTable()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    if (!confirmAction(tr("Are you sure you want to truncate table %1?\nAll data will be lost!").arg(tableName))) {
        return;
    }

    QSqlQuery query;
    if (!query.exec(QString("TRUNCATE TABLE %1").arg(tableName))) {
        showError(tr("Truncate Table Failed"), query.lastError());
        return;
    }

    statusBar->showMessage(tr("Table %1 truncated").arg(tableName), 3000);
    refreshData();
}

// Data operations
void DatabaseAdmin::executeQuery()
{
    if (!checkDatabaseConnection()) return;

    QString queryText = queryEditor->toPlainText().trimmed();
    if (queryText.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a SQL query"));
        return;
    }

    executeAndShowQuery(queryText);
    statusBar->showMessage(tr("Query executed"), 2000);
}

void DatabaseAdmin::exportToCSV()
{
    if (sqlModel->rowCount() == 0) {
        QMessageBox::warning(this, tr("Warning"), tr("No data to export"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export to CSV"),
                                                    lastDir, tr("CSV Files (*.csv)"));
    if (fileName.isEmpty()) return;

    lastDir = QFileInfo(fileName).path();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Cannot open file for writing:\n%1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);

    // Write headers
    for (int col = 0; col < sqlModel->columnCount(); ++col) {
        if (col > 0) out << ",";
        out << escapeCSVValue(sqlModel->headerData(col, Qt::Horizontal).toString());
    }
    out << "\n";

    // Write data
    for (int row = 0; row < sqlModel->rowCount(); ++row) {
        for (int col = 0; col < sqlModel->columnCount(); ++col) {
            if (col > 0) out << ",";
            out << escapeCSVValue(sqlModel->data(sqlModel->index(row, col)).toString());
        }
        out << "\n";
    }

    file.close();
    statusBar->showMessage(tr("Data exported to %1").arg(fileName), 3000);
}

void DatabaseAdmin::importFromCSV()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Import from CSV"),
                                                    lastDir, tr("CSV Files (*.csv)"));
    if (fileName.isEmpty()) return;

    lastDir = QFileInfo(fileName).path();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Cannot open file for reading:\n%1").arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QStringList headers;
    QStringList columnNames;
    int lineNumber = 0;
    int importedRows = 0;

    // Get column names from the table
    QSqlQuery columnQuery(QString("SHOW COLUMNS FROM %1").arg(tableName));
    while (columnQuery.next()) {
        columnNames << columnQuery.value(0).toString();
    }

    QSqlDatabase::database().transaction();
    QSqlQuery insertQuery;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;

        QStringList values = parseCSVLine(line);

        // First line is headers
        if (lineNumber == 0) {
            headers = values;
            lineNumber++;
            continue;
        }

        // Prepare INSERT statement
        if (insertQuery.lastQuery().isEmpty()) {
            QStringList placeholders;
            for (int i = 0; i < columnNames.size(); ++i) {
                placeholders << "?";
            }
            QString queryStr = QString("INSERT INTO %1 (%2) VALUES (%3)")
                .arg(tableName)
                .arg(columnNames.join(", "))
                .arg(placeholders.join(", "));
            insertQuery.prepare(queryStr);
        }

        // Bind values (simple mapping by position)
        for (int i = 0; i < qMin(values.size(), columnNames.size()); ++i) {
            insertQuery.bindValue(i, values[i]);
        }

        if (!insertQuery.exec()) {
            QMessageBox::critical(this, tr("Error"),
                                tr("Failed to import row %1:\n%2")
                                    .arg(lineNumber)
                                    .arg(insertQuery.lastError().text()));
            QSqlDatabase::database().rollback();
            file.close();
            return;
        }

        importedRows++;
        lineNumber++;
    }

    QSqlDatabase::database().commit();
    file.close();

    statusBar->showMessage(tr("Imported %1 rows from %2").arg(importedRows).arg(fileName), 3000);
    refreshData();
}

void DatabaseAdmin::copyData()
{
    QItemSelectionModel *selection = tableView->selectionModel();
    if (!selection->hasSelection()) return;

    QModelIndexList indexes = selection->selectedIndexes();
    if (indexes.isEmpty()) return;

    // Sort indexes by row and column
    std::sort(indexes.begin(), indexes.end(), [](const QModelIndex &a, const QModelIndex &b) {
        if (a.row() == b.row()) return a.column() < b.column();
        return a.row() < b.row();
    });

    QString clipboardText;
    int currentRow = indexes.first().row();
    int currentCol = indexes.first().column();

    for (const QModelIndex &index : indexes) {
        if (index.row() != currentRow) {
            clipboardText += "\n";
            currentRow = index.row();
            currentCol = index.column();
        } else if (index.column() != currentCol) {
            clipboardText += "\t";
            currentCol = index.column();
        }

        clipboardText += index.data().toString();
    }

    QApplication::clipboard()->setText(clipboardText);
    statusBar->showMessage(tr("Copied %1 cells").arg(indexes.size()), 2000);
}

void DatabaseAdmin::pasteData()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    QString clipboardText = QApplication::clipboard()->text();
    if (clipboardText.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Clipboard is empty"));
        return;
    }

    // Get column names from the table
    QStringList columnNames;
    QSqlQuery columnQuery(QString("SHOW COLUMNS FROM %1").arg(tableName));
    while (columnQuery.next()) {
        columnNames << columnQuery.value(0).toString();
    }

    QStringList rows = clipboardText.split('\n', Qt::SkipEmptyParts);
    int pastedRows = 0;

    QSqlDatabase::database().transaction();
    QSqlQuery insertQuery;

    for (const QString &row : rows) {
        QStringList values = row.split('\t');

        // Prepare INSERT statement
        if (insertQuery.lastQuery().isEmpty()) {
            QStringList placeholders;
            for (int i = 0; i < columnNames.size(); ++i) {
                placeholders << "?";
            }
            QString queryStr = QString("INSERT INTO %1 (%2) VALUES (%3)")
                .arg(tableName)
                .arg(columnNames.join(", "))
                .arg(placeholders.join(", "));
            insertQuery.prepare(queryStr);
        }

        // Bind values (simple mapping by position)
        for (int i = 0; i < qMin(values.size(), columnNames.size()); ++i) {
            insertQuery.bindValue(i, values[i]);
        }

        if (!insertQuery.exec()) {
            QMessageBox::critical(this, tr("Error"),
                                tr("Failed to paste row %1:\n%2")
                                    .arg(pastedRows + 1)
                                    .arg(insertQuery.lastError().text()));
            QSqlDatabase::database().rollback();
            return;
        }

        pastedRows++;
    }

    QSqlDatabase::database().commit();
    statusBar->showMessage(tr("Pasted %1 rows").arg(pastedRows), 3000);
    refreshData();
}

void DatabaseAdmin::deleteSelectedRows()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    QItemSelectionModel *selection = tableView->selectionModel();
    if (!selection->hasSelection()) {
        QMessageBox::warning(this, tr("Warning"), tr("No rows selected"));
        return;
    }

    QModelIndexList selectedRows = selection->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No complete rows selected"));
        return;
    }

    if (!confirmAction(tr("Are you sure you want to delete %1 selected rows?").arg(selectedRows.size()))) {
        return;
    }

    // Get primary key column
    QString primaryKey;
    QSqlQuery pkQuery(QString("SHOW KEYS FROM %1 WHERE Key_name = 'PRIMARY'").arg(tableName));
    if (pkQuery.next()) {
        primaryKey = pkQuery.value("Column_name").toString();
    } else {
        QMessageBox::warning(this, tr("Warning"),
                            tr("Cannot determine primary key for table %1").arg(tableName));
        return;
    }

    QSqlDatabase::database().transaction();
    int deletedRows = 0;

    for (const QModelIndex &index : selectedRows) {
        QVariant pkValue = sqlModel->data(sqlModel->index(index.row(), 0)); // Assuming PK is first column

        QSqlQuery deleteQuery;
        if (!deleteQuery.exec(QString("DELETE FROM %1 WHERE %2 = '%3'")
                                .arg(tableName)
                                .arg(primaryKey)
                                .arg(pkValue.toString()))) {
            showError(tr("Delete Failed"), deleteQuery.lastError());
            QSqlDatabase::database().rollback();
            return;
        }

        deletedRows++;
    }

    QSqlDatabase::database().commit();
    statusBar->showMessage(tr("Deleted %1 rows").arg(deletedRows), 3000);
    refreshData();
}

void DatabaseAdmin::insertRow()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    // Get column information
    QSqlQuery columnQuery(QString("SHOW COLUMNS FROM %1").arg(tableName));
    QStringList columnNames;
    QStringList columnTypes;

    while (columnQuery.next()) {
        columnNames << columnQuery.value(0).toString();
        columnTypes << columnQuery.value(1).toString();
    }

    // Create a dialog to input values
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Insert Row"));
    QFormLayout layout(&dialog);

    QList<QLineEdit*> lineEdits;
    for (int i = 0; i < columnNames.size(); ++i) {
        QLineEdit *lineEdit = new QLineEdit(&dialog);
        layout.addRow(columnNames[i] + " (" + columnTypes[i] + ")", lineEdit);
        lineEdits << lineEdit;
    }

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                            Qt::Horizontal, &dialog);
    layout.addRow(&buttons);

    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // Prepare INSERT statement
    QStringList placeholders;
    for (int i = 0; i < columnNames.size(); ++i) {
        placeholders << "?";
    }
    QString queryStr = QString("INSERT INTO %1 (%2) VALUES (%3)")
        .arg(tableName)
        .arg(columnNames.join(", "))
        .arg(placeholders.join(", "));
    QSqlQuery query;
    query.prepare(queryStr);

    for (int i = 0; i < lineEdits.size(); ++i) {
        query.bindValue(i, lineEdits[i]->text());
    }

    if (!query.exec()) {
        showError(tr("Insert Failed"), query.lastError());
        return;
    }

    statusBar->showMessage(tr("Row inserted"), 3000);
    refreshData();
}

// View operations
void DatabaseAdmin::filterData()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    bool ok;
    QString filter = QInputDialog::getText(this, tr("Filter Data"),
                                        tr("Enter WHERE clause (without WHERE):"),
                                        QLineEdit::Normal, "", &ok);
    if (!ok || filter.isEmpty()) return;

    executeAndShowQuery(QString("SELECT * FROM %1 WHERE %2").arg(tableName).arg(filter));
    statusBar->showMessage(tr("Filter applied"), 2000);
}

void DatabaseAdmin::sortData()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No table selected"));
        return;
    }

    bool ok;
    QString sort = QInputDialog::getText(this, tr("Sort Data"),
                                        tr("Enter ORDER BY clause (without ORDER BY):"),
                                        QLineEdit::Normal, "", &ok);
    if (!ok || sort.isEmpty()) return;

    executeAndShowQuery(QString("SELECT * FROM %1 ORDER BY %2").arg(tableName).arg(sort));
    statusBar->showMessage(tr("Sort applied"), 2000);
}

void DatabaseAdmin::resetView()
{
    if (!checkDatabaseConnection()) return;

    QString tableName = currentTableName();
    if (tableName.isEmpty()) {
        showTables();
        return;
    }

    executeAndShowQuery(QString("SELECT * FROM %1").arg(tableName));
    statusBar->showMessage(tr("View reset"), 2000);
}

// Settings
void DatabaseAdmin::showPreferences()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Preferences"));

    QFormLayout layout(&dialog);

    QCheckBox *autoRefreshCheck = new QCheckBox(&dialog);
    autoRefreshCheck->setChecked(autoRefresh);
    layout.addRow(tr("Auto-refresh data:"), autoRefreshCheck);

    QSpinBox *refreshIntervalSpin = new QSpinBox(&dialog);
    refreshIntervalSpin->setRange(1, 3600);
    refreshIntervalSpin->setValue(refreshInterval);
    refreshIntervalSpin->setSuffix(tr(" seconds"));
    layout.addRow(tr("Refresh interval:"), refreshIntervalSpin);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                            Qt::Horizontal, &dialog);
    layout.addRow(&buttons);

    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        autoRefresh = autoRefreshCheck->isChecked();
        refreshInterval = refreshIntervalSpin->value();

        // TODO: Implement auto-refresh timer if needed
        statusBar->showMessage(tr("Preferences saved"), 2000);
    }
}

void DatabaseAdmin::about()
{
    QMessageBox::about(this, tr("About Database Administrator"),
                    tr("<h2>Database Administrator 1.0</h2>"
                        "<p>A simple database administration tool built with Qt.</p>"
                        "<p>Supports MySQL databases with Excel-like functionality.</p>"
                        "<p>&copy; 2023 Your Company</p>"));
}

// Helper methods
bool DatabaseAdmin::checkDatabaseConnection()
{
    if (!QSqlDatabase::database().isOpen()) {
        QMessageBox::warning(this, tr("Warning"), tr("Not connected to database"));
        return false;
    }
    return true;
}

QStringList DatabaseAdmin::getTableList()
{
    QStringList tables;
    if (!checkDatabaseConnection()) return tables;

    QSqlQuery query("SHOW TABLES");
    while (query.next()) {
        tables << query.value(0).toString();
    }

    return tables;
}

QString DatabaseAdmin::currentTableName() const
{
    QString queryText = sqlModel->query().executedQuery();
    if (queryText.startsWith("SELECT * FROM ")) {
        return queryText.mid(14).section(' ', 0, 0);
    } else if (queryText.startsWith("SELECT")) {
        // Complex query, no simple table name
        return QString();
    } else if (queryText.startsWith("SHOW TABLES")) {
        // Showing tables list
        return QString();
    }

    return QString();
}

QStringList DatabaseAdmin::parseCSVLine(const QString &line)
{
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar ch = line.at(i);
        if (ch == '"') {
            if (inQuotes && i < line.length() - 1 && line.at(i + 1) == '"') {
                field += '"';
                i++; // Skip next character
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            fields.append(field);
            field.clear();
        } else {
            field += ch;
        }
    }
    fields.append(field);

    return fields;
}

QString DatabaseAdmin::escapeCSVValue(const QString &value) const
{
    if (value.contains('"') || value.contains(',') || value.contains('\n')) {
        return '"' + QString(value).replace("\"", "\"\"") + '"';
    }
    return value;
}

void DatabaseAdmin::updateRowCount()
{
    statusBar->showMessage(tr("%1 rows").arg(sqlModel->rowCount()), 2000);
}

void DatabaseAdmin::executeAndShowQuery(const QString &query)
{
    QSqlQuery sqlQuery;
    if (!sqlQuery.exec(query)) {
        showError(tr("Query Failed"), sqlQuery.lastError());
        return;
    }

    // Используем std::move(), чтобы передать sqlQuery по перемещению
    sqlModel->setQuery(std::move(sqlQuery));

    if (sqlModel->lastError().isValid()) {
        showError(tr("Query Failed"), sqlModel->lastError());
        return;
    }

    updateRowCount();
}

void DatabaseAdmin::showError(const QString &title, const QSqlError &error)
{
    QMessageBox::critical(this, title,
                        tr("Database Error:\n%1\n\nSQL: %2")
                            .arg(error.text())
                            .arg(error.databaseText()));
}

bool DatabaseAdmin::confirmAction(const QString &message)
{
    return QMessageBox::question(this, tr("Confirm Action"), message,
                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}


