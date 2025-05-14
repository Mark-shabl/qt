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
#include <QDebug>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSqlTableModel>
#include <QSqlRecord>

DatabaseAdmin::DatabaseAdmin(QWidget *parent)
    : QMainWindow(parent),
    sqlModel(new QSqlTableModel(this)),
    settings(new QSettings("DatabaseAdmin", "QtDBAdmin", this))
{
    // Проверка доступности драйвера SQLite
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        QMessageBox::critical(nullptr, "Ошибка", "Драйвер SQLite не доступен");
        exit(1);
    }

    setupUI();
    loadSettings();
}

DatabaseAdmin::~DatabaseAdmin()
{
    saveSettings();
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }
}

void DatabaseAdmin::setupUI()
{
    // Основные виджеты
    tableView = new QTableView(this);
    queryEditor = new QTextEdit(this);
    statusBar = new QStatusBar(this);

    // Настройка модели
    sqlModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    tableView->setModel(sqlModel);

    // Док-окно для запросов
    queryDock = new QDockWidget(tr("SQL Запрос"), this);
    queryDock->setWidget(queryEditor);
    addDockWidget(Qt::BottomDockWidgetArea, queryDock);

    // Настройка главного окна
    setCentralWidget(tableView);
    setStatusBar(statusBar);
    setWindowTitle(tr("Администратор Баз Данных"));
    resize(1000, 700);

    // Настройки таблицы
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setEditTriggers(QAbstractItemView::DoubleClicked);
    tableView->setSortingEnabled(true);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    // Создание меню и панелей инструментов
    createMenus();
    createToolBars();
}

void DatabaseAdmin::createMenus()
{
    // Меню "Файл"
    QMenu *fileMenu = menuBar()->addMenu(tr("&Файл"));
    connectAction = fileMenu->addAction(tr("&Подключиться..."), this, &DatabaseAdmin::connectToDatabase);
    disconnectAction = fileMenu->addAction(tr("&Отключиться"), this, &DatabaseAdmin::disconnectFromDatabase);
    fileMenu->addSeparator();
    exportAction = fileMenu->addAction(tr("&Экспорт в CSV..."), this, &DatabaseAdmin::exportToCSV);
    importAction = fileMenu->addAction(tr("&Импорт из CSV..."), this, &DatabaseAdmin::importFromCSV);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Выход"), qApp, &QApplication::closeAllWindows);

    QMenu *dbMenu = menuBar()->addMenu(tr("&База данных"));
    dbMenu->addAction(tr("&Создать БД..."), this, &DatabaseAdmin::createDatabase);

    // Меню "Правка"
    QMenu *editMenu = menuBar()->addMenu(tr("&Правка"));
    refreshAction = editMenu->addAction(tr("&Обновить"), this, &DatabaseAdmin::refreshData);
    refreshAction->setShortcut(QKeySequence::Refresh);
    editMenu->addSeparator();
    copyAction = editMenu->addAction(tr("&Копировать"), this, &DatabaseAdmin::copyData);
    copyAction->setShortcut(QKeySequence::Copy);
    deleteAction = editMenu->addAction(tr("&Удалить строки"), this, &DatabaseAdmin::deleteSelectedRows);
    insertAction = editMenu->addAction(tr("&Вставить строку"), this, &DatabaseAdmin::insertRow);
    editMenu->addSeparator();
    submitAction = editMenu->addAction(tr("&Применить изменения"), this, &DatabaseAdmin::submitChanges);
    revertAction = editMenu->addAction(tr("&Отменить изменения"), this, &DatabaseAdmin::revertChanges);

    // Меню "Таблица"
    QMenu *tableMenu = menuBar()->addMenu(tr("&Таблица"));
    showTablesAction = tableMenu->addAction(tr("&Показать таблицы"), this, &DatabaseAdmin::showTables);
    tableMenu->addSeparator();
    tableMenu->addAction(tr("&Создать таблицу..."), this, &DatabaseAdmin::createTable);
    tableMenu->addAction(tr("&Удалить таблицу..."), this, &DatabaseAdmin::dropTable);

    // Меню "Вид"
    QMenu *viewMenu = menuBar()->addMenu(tr("&Вид"));
    filterAction = viewMenu->addAction(tr("&Фильтровать данные..."), this, &DatabaseAdmin::filterData);
    sortAction = viewMenu->addAction(tr("&Сортировать данные..."), this, &DatabaseAdmin::sortData);
    resetAction = viewMenu->addAction(tr("&Сбросить вид"), this, &DatabaseAdmin::resetView);

    // Меню "Запрос"
    QMenu *queryMenu = menuBar()->addMenu(tr("&Запрос"));
    executeAction = queryMenu->addAction(tr("&Выполнить"), this, &DatabaseAdmin::executeQuery);
    executeAction->setShortcut(Qt::Key_F5);
}

void DatabaseAdmin::createDatabase()
{
    bool ok;
    QString dbName = QInputDialog::getText(this,
                                           tr("Создание базы данных"),
                                           tr("Введите имя файла для новой базы данных:"),
                                           QLineEdit::Normal,
                                           "new_database.db",
                                           &ok);

    if (!ok || dbName.isEmpty()) return;

    // Добавляем расширение .db если его нет
    if (!dbName.endsWith(".db")) dbName += ".db";

    QString fullPath = QDir::current().absoluteFilePath(dbName);

    // Удаляем старое соединение если есть
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(fullPath);

    if (!db.open()) {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось создать базу данных:\n%1").arg(db.lastError().text()));
        return;
    }

    // Создаем простую таблицу для примера
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, name TEXT)")) {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось создать таблицу:\n%1").arg(query.lastError().text()));
        return;
    }

    QMessageBox::information(this, tr("Успех"),
                             tr("База данных успешно создана:\n%1").arg(fullPath));

    // Подключаемся к новой БД
    connectToDatabase();
}

void DatabaseAdmin::refreshDatabaseList()
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);

    if (query.exec("SHOW DATABASES")) {
        QStringList databases;
        while (query.next()) {
            databases << query.value(0).toString();
        }
        qDebug() << "Доступные базы данных:" << databases;
    } else {
        qWarning() << "Ошибка при получении списка баз данных:" << query.lastError();
    }
}

bool DatabaseAdmin::databaseExists(const QString &dbName)
{
    QSqlQuery query;
    query.exec("SHOW DATABASES");
    while (query.next()) {
        if (query.value(0).toString() == dbName)
            return true;
    }
    return false;
}

QStringList DatabaseAdmin::getDatabaseList() const
{
    QStringList databases;
    QSqlQuery query("SHOW DATABASES");
    while (query.next()) {
        databases << query.value(0).toString();
    }
    return databases;
}

void DatabaseAdmin::dropDatabase()
{
    bool ok;
    QString dbName = QInputDialog::getItem(this,
                                           tr("Удаление базы данных"),
                                           tr("Выберите базу данных для удаления:"),
                                           getDatabaseList(),
                                           0,
                                           false,
                                           &ok);

    if (!ok || dbName.isEmpty())
        return;

    if (QMessageBox::question(this,
                              tr("Подтверждение"),
                              tr("Вы уверены, что хотите удалить базу данных '%1'?").arg(dbName))
        != QMessageBox::Yes)
        return;

    QSqlQuery query;
    if (!query.exec(QString("DROP DATABASE `%1`").arg(dbName))) {
        QMessageBox::critical(this,
                              tr("Ошибка"),
                              tr("Не удалось удалить базу данных:\n") + query.lastError().text());
    } else {
        QMessageBox::information(this,
                                 tr("Успех"),
                                 tr("База данных '%1' успешно удалена").arg(dbName));
    }
}



void DatabaseAdmin::createToolBars()
{
    // Панель инструментов "База данных"
    QToolBar *dbToolBar = addToolBar(tr("База данных"));
    dbToolBar->addAction(connectAction);
    dbToolBar->addAction(disconnectAction);
    dbToolBar->addSeparator();
    dbToolBar->addAction(refreshAction);

    // Панель инструментов "Таблица"
    QToolBar *tableToolBar = addToolBar(tr("Таблица"));
    tableToolBar->addAction(showTablesAction);
    tableToolBar->addSeparator();
    tableToolBar->addAction(exportAction);
    tableToolBar->addAction(importAction);

    // Панель инструментов "Данные"
    QToolBar *dataToolBar = addToolBar(tr("Данные"));
    dataToolBar->addAction(copyAction);
    dataToolBar->addAction(deleteAction);
    dataToolBar->addAction(insertAction);
    dataToolBar->addSeparator();
    dataToolBar->addAction(submitAction);
    dataToolBar->addAction(revertAction);

    // Панель инструментов "Запрос"
    QToolBar *queryToolBar = addToolBar(tr("Запрос"));
    queryToolBar->addAction(executeAction);
}

void DatabaseAdmin::connectToDatabase()
{
    // Закрываем предыдущее соединение, если оно есть
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }

    QString dbPath = QFileDialog::getOpenFileName(this,
                                                  tr("Выберите файл базы данных"),
                                                  lastDir,
                                                  tr("Базы данных SQLite (*.db *.sqlite)"));

    if (dbPath.isEmpty()) return;

    lastDir = QFileInfo(dbPath).path();

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось открыть базу данных:\n%1").arg(db.lastError().text()));
        return;
    }

    // Настраиваем модель после подключения
    sqlModel->setTable("");
    sqlModel->select();

    statusBar->showMessage(tr("Подключено к %1").arg(dbPath), 3000);
    showTables();
}
void DatabaseAdmin::disconnectFromDatabase()
{
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
        sqlModel->clear();
        statusBar->showMessage(tr("Отключено от базы данных"), 3000);
    }
}

void DatabaseAdmin::showTables()
{
    if (!QSqlDatabase::database().isOpen()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("База данных не подключена"));
        return;
    }

    // Получаем список всех таблиц
    QStringList tables = QSqlDatabase::database().tables(QSql::Tables);

    if (tables.isEmpty()) {
        QMessageBox::information(this, tr("Информация"),
                                 tr("В базе данных нет таблиц"));
        return;
    }

    // Показываем диалог выбора таблицы
    bool ok;
    QString tableName = QInputDialog::getItem(this,
                                              tr("Выбор таблицы"),
                                              tr("Выберите таблицу:"),
                                              tables,
                                              0,
                                              false,
                                              &ok);
    if (!ok || tableName.isEmpty()) return;

    // Устанавливаем выбранную таблицу в модель
    sqlModel->setTable(tableName);
    sqlModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    if (!sqlModel->select()) {
        showError(tr("Ошибка загрузки таблицы"), sqlModel->lastError());
        return;
    }

    // Обновляем заголовки столбцов
    for (int i = 0; i < sqlModel->columnCount(); ++i) {
        sqlModel->setHeaderData(i, Qt::Horizontal, sqlModel->record().fieldName(i));
    }

    statusBar->showMessage(tr("Загружена таблица: %1").arg(tableName), 2000);
}

void DatabaseAdmin::createTable()
{
    bool ok;
    QString tableName = QInputDialog::getText(this, tr("Создание таблицы"),
                                              tr("Имя таблицы:"), QLineEdit::Normal, "", &ok);
    if (!ok || tableName.isEmpty()) return;

    QString columns = QInputDialog::getMultiLineText(this, tr("Создание таблицы"),
                                                     tr("Определения столбцов (по одному на строку):"),
                                                     "id INTEGER PRIMARY KEY AUTOINCREMENT", &ok);
    if (!ok || columns.isEmpty()) return;

    QStringList columnDefs = columns.split('\n', Qt::SkipEmptyParts);
    QString queryStr = QString("CREATE TABLE %1 (%2)").arg(tableName).arg(columnDefs.join(", "));

    QSqlQuery query;
    if (!query.exec(queryStr)) {
        showError(tr("Ошибка создания таблицы"), query.lastError());
        return;
    }

    // Обновляем список таблиц
    showTables();
    statusBar->showMessage(tr("Таблица %1 создана").arg(tableName), 3000);
}

void DatabaseAdmin::dropTable()
{
    QStringList tables = QSqlDatabase::database().tables(QSql::Tables);
    if (tables.isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет таблиц для удаления"));
        return;
    }

    bool ok;
    QString tableName = QInputDialog::getItem(this, tr("Удаление таблицы"),
                                              tr("Выберите таблицу для удаления:"), tables, 0, false, &ok);
    if (!ok || tableName.isEmpty()) return;

    if (!confirmAction(tr("Вы уверены, что хотите удалить таблицу %1?").arg(tableName))) {
        return;
    }

    QSqlQuery query;
    if (!query.exec(QString("DROP TABLE %1").arg(tableName))) {
        showError(tr("Ошибка удаления таблицы"), query.lastError());
        return;
    }

    // Если удаляли текущую таблицу - очищаем модель
    if (sqlModel->tableName() == tableName) {
        sqlModel->clear();
    }

    statusBar->showMessage(tr("Таблица %1 удалена").arg(tableName), 3000);
}

void DatabaseAdmin::executeQuery()
{
    QString queryText = queryEditor->toPlainText().trimmed();
    if (queryText.isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Введите SQL-запрос"));
        return;
    }

    QSqlQuery query;
    if (!query.exec(queryText)) {
        showError(tr("Ошибка выполнения запроса"), query.lastError());
        return;
    }

    // Если это SELECT запрос - обновляем модель
    if (queryText.startsWith("SELECT", Qt::CaseInsensitive)) {
        sqlModel->setQuery(query);
        if (sqlModel->lastError().isValid()) {
            showError(tr("Ошибка загрузки результатов"), sqlModel->lastError());
            return;
        }
    } else {
        // Для других запросов обновляем текущую таблицу
        if (!sqlModel->tableName().isEmpty()) {
            sqlModel->select();
        }
    }

    statusBar->showMessage(tr("Запрос выполнен. Строк: %1").arg(sqlModel->rowCount()), 2000);
}

void DatabaseAdmin::submitChanges()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет активной таблицы"));
        return;
    }

    if (!sqlModel->submitAll()) {
        showError(tr("Ошибка сохранения изменений"), sqlModel->lastError());
    } else {
        statusBar->showMessage(tr("Изменения сохранены"), 2000);
    }
}

void DatabaseAdmin::revertChanges()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет активной таблицы"));
        return;
    }

    sqlModel->revertAll();
    statusBar->showMessage(tr("Изменения отменены"), 2000);
}

void DatabaseAdmin::refreshData()
{
    if (sqlModel->tableName().isEmpty()) {
        showTables();
        return;
    }

    if (!sqlModel->select()) {
        showError(tr("Ошибка обновления данных"), sqlModel->lastError());
        return;
    }

    statusBar->showMessage(tr("Данные обновлены"), 2000);
}

void DatabaseAdmin::exportToCSV()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет активной таблицы"));
        return;
    }

    if (sqlModel->rowCount() == 0) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет данных для экспорта"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Экспорт в CSV"),
                                                    lastDir, tr("CSV файлы (*.csv)"));
    if (fileName.isEmpty()) return;

    lastDir = QFileInfo(fileName).path();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось открыть файл для записи:\n%1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);

    // Запись заголовков
    for (int col = 0; col < sqlModel->columnCount(); ++col) {
        if (col > 0) out << ",";
        out << "\"" << sqlModel->headerData(col, Qt::Horizontal).toString().replace("\"", "\"\"") << "\"";
    }
    out << "\n";

    // Запись данных
    for (int row = 0; row < sqlModel->rowCount(); ++row) {
        for (int col = 0; col < sqlModel->columnCount(); ++col) {
            if (col > 0) out << ",";
            out << "\"" << sqlModel->data(sqlModel->index(row, col)).toString().replace("\"", "\"\"") << "\"";
        }
        out << "\n";
    }

    file.close();
    statusBar->showMessage(tr("Данные экспортированы в %1").arg(fileName), 3000);
}

void DatabaseAdmin::importFromCSV()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Сначала выберите таблицу"));
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Импорт из CSV"),
                                                    lastDir, tr("CSV файлы (*.csv)"));
    if (fileName.isEmpty()) return;

    lastDir = QFileInfo(fileName).path();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Ошибка"),
                              tr("Не удалось открыть файл для чтения:\n%1").arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QStringList headers;
    int lineNumber = 0;
    int importedRows = 0;

    // Получение информации о столбцах таблицы
    QStringList columnNames;
    for (int i = 0; i < sqlModel->columnCount(); ++i) {
        columnNames << sqlModel->headerData(i, Qt::Horizontal).toString();
    }

    QSqlDatabase::database().transaction();
    QSqlQuery insertQuery;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;

        QStringList values;
        QString value;
        bool inQuotes = false;

        // Простой парсер CSV
        for (int i = 0; i < line.length(); ++i) {
            QChar ch = line.at(i);
            if (ch == '"') {
                inQuotes = !inQuotes;
            } else if (ch == ',' && !inQuotes) {
                values << value;
                value.clear();
            } else {
                value += ch;
            }
        }
        values << value;

        // Первая строка - заголовки
        if (lineNumber == 0) {
            headers = values;
            lineNumber++;
            continue;
        }

        // Подготовка INSERT запроса
        if (insertQuery.lastQuery().isEmpty()) {
            QString queryStr = QString("INSERT INTO %1 (%2) VALUES (%3)")
            .arg(sqlModel->tableName())
                .arg(columnNames.join(", "))
                .arg(QString("?, ").repeated(columnNames.size()).chopped(2));
            insertQuery.prepare(queryStr);
        }

        // Привязка значений
        for (int i = 0; i < qMin(values.size(), columnNames.size()); ++i) {
            insertQuery.bindValue(i, values[i].trimmed());
        }

        if (!insertQuery.exec()) {
            QMessageBox::critical(this, tr("Ошибка"),
                                  tr("Не удалось импортировать строку %1:\n%2")
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

    // Обновляем данные после импорта
    sqlModel->select();
    statusBar->showMessage(tr("Импортировано %1 строк из %2").arg(importedRows).arg(fileName), 3000);
}

void DatabaseAdmin::copyData()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет активной таблицы"));
        return;
    }

    QItemSelectionModel *selection = tableView->selectionModel();
    if (!selection->hasSelection()) return;

    QModelIndexList indexes = selection->selectedIndexes();
    if (indexes.isEmpty()) return;

    // Сортировка индексов по строкам и столбцам
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
    statusBar->showMessage(tr("Скопировано %1 ячеек").arg(indexes.size()), 2000);
}

void DatabaseAdmin::deleteSelectedRows()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет активной таблицы"));
        return;
    }

    QItemSelectionModel *selection = tableView->selectionModel();
    if (!selection->hasSelection()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Строки не выбраны"));
        return;
    }

    QModelIndexList selectedRows = selection->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Не выбрано ни одной строки"));
        return;
    }

    if (!confirmAction(tr("Вы уверены, что хотите удалить %1 выбранных строк?").arg(selectedRows.size()))) {
        return;
    }

    // Получение первичного ключа (предполагаем, что первый столбец - первичный ключ)
    QString primaryKey = sqlModel->headerData(0, Qt::Horizontal).toString();

    QSqlDatabase::database().transaction();
    int deletedRows = 0;

    for (const QModelIndex &index : selectedRows) {
        QVariant pkValue = sqlModel->data(sqlModel->index(index.row(), 0));

        QSqlQuery deleteQuery;
        deleteQuery.prepare(QString("DELETE FROM %1 WHERE %2 = ?")
                                .arg(sqlModel->tableName())
                                .arg(primaryKey));
        deleteQuery.bindValue(0, pkValue);

        if (!deleteQuery.exec()) {
            showError(tr("Ошибка удаления"), deleteQuery.lastError());
            QSqlDatabase::database().rollback();
            return;
        }

        deletedRows++;
    }

    QSqlDatabase::database().commit();

    // Обновляем данные после удаления
    sqlModel->select();
    statusBar->showMessage(tr("Удалено %1 строк").arg(deletedRows), 3000);
}

void DatabaseAdmin::insertRow()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет активной таблицы"));
        return;
    }

    // Получение информации о столбцах
    QStringList columnNames;
    for (int i = 0; i < sqlModel->columnCount(); ++i) {
        columnNames << sqlModel->headerData(i, Qt::Horizontal).toString();
    }

    // Создание диалога для ввода значений
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Добавление строки"));
    QFormLayout layout(&dialog);

    QList<QLineEdit*> lineEdits;
    for (const QString &column : columnNames) {
        QLineEdit *lineEdit = new QLineEdit(&dialog);
        layout.addRow(column, lineEdit);
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

    // Вставка новой строки
    int row = sqlModel->rowCount();
    if (!sqlModel->insertRow(row)) {
        showError(tr("Ошибка вставки строки"), sqlModel->lastError());
        return;
    }

    // Установка значений
    for (int i = 0; i < lineEdits.size(); ++i) {
        if (i < sqlModel->columnCount()) {
            sqlModel->setData(sqlModel->index(row, i), lineEdits[i]->text());
        }
    }

    statusBar->showMessage(tr("Строка добавлена (не забудьте применить изменения)"), 3000);
}

void DatabaseAdmin::filterData()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет активной таблицы"));
        return;
    }

    bool ok;
    QString filter = QInputDialog::getText(this, tr("Фильтрация данных"),
                                           tr("Введите условие WHERE (без слова WHERE):"),
                                           QLineEdit::Normal, "", &ok);
    if (!ok || filter.isEmpty()) return;

    sqlModel->setFilter(filter);
    if (!sqlModel->select()) {
        showError(tr("Ошибка фильтрации"), sqlModel->lastError());
        return;
    }

    statusBar->showMessage(tr("Фильтр применен"), 2000);
}

void DatabaseAdmin::sortData()
{
    if (sqlModel->tableName().isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Нет активной таблицы"));
        return;
    }

    bool ok;
    QString sort = QInputDialog::getText(this, tr("Сортировка данных"),
                                         tr("Введите условие ORDER BY (без слов ORDER BY):"),
                                         QLineEdit::Normal, "", &ok);
    if (!ok || sort.isEmpty()) return;

    sqlModel->setSort(0, Qt::AscendingOrder); // Сброс предыдущей сортировки
    sqlModel->setFilter(QString()); // Сброс фильтра

    QString query = QString("SELECT * FROM %1 ORDER BY %2").arg(sqlModel->tableName()).arg(sort);
    sqlModel->setQuery(query);

    if (sqlModel->lastError().isValid()) {
        showError(tr("Ошибка сортировки"), sqlModel->lastError());
        return;
    }

    statusBar->showMessage(tr("Сортировка применена"), 2000);
}

void DatabaseAdmin::resetView()
{
    if (sqlModel->tableName().isEmpty()) {
        showTables();
        return;
    }

    sqlModel->setFilter(QString());
    sqlModel->setSort(-1, Qt::AscendingOrder); // Сброс сортировки

    if (!sqlModel->select()) {
        showError(tr("Ошибка сброса вида"), sqlModel->lastError());
        return;
    }

    statusBar->showMessage(tr("Вид сброшен"), 2000);
}

void DatabaseAdmin::loadSettings()
{
    settings->beginGroup("MainWindow");
    restoreGeometry(settings->value("geometry").toByteArray());
    restoreState(settings->value("windowState").toByteArray());
    settings->endGroup();

    settings->beginGroup("Preferences");
    lastDir = settings->value("lastDir", QDir::homePath()).toString();
    settings->endGroup();
}

void DatabaseAdmin::saveSettings()
{
    settings->beginGroup("MainWindow");
    settings->setValue("geometry", saveGeometry());
    settings->setValue("windowState", saveState());
    settings->endGroup();

    settings->beginGroup("Preferences");
    settings->setValue("lastDir", lastDir);
    settings->endGroup();
}

void DatabaseAdmin::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }
    event->accept();
}

void DatabaseAdmin::showError(const QString &title, const QSqlError &error)
{
    QMessageBox::critical(this, title,
                          tr("Ошибка базы данных:\n%1\n\nSQL: %2")
                              .arg(error.text())
                              .arg(error.databaseText()));
}

bool DatabaseAdmin::confirmAction(const QString &message)
{
    return QMessageBox::question(this, tr("Подтверждение действия"), message,
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}
