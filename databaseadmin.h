#ifndef DATABASEADMIN_H
#define DATABASEADMIN_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QSettings>
#include <QSqlRecord>  // Добавлено для работы с QSqlRecord

class QTableView;
class QTextEdit;
class QStatusBar;
class QDockWidget;
class QMenu;
class QToolBar;
class QAction;

class DatabaseAdmin : public QMainWindow
{
    Q_OBJECT

public:
    DatabaseAdmin(QWidget *parent = nullptr);
    ~DatabaseAdmin();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Database operations
    void createDatabase();
    void dropDatabase();  // Добавлено
    void connectToDatabase();
    void disconnectFromDatabase();
    void refreshData();

    // Table operations
    void showTables();
    void createTable();
    void dropTable();

    // Data operations
    void executeQuery();
    void exportToCSV();
    void importFromCSV();
    void copyData();
    void deleteSelectedRows();
    void insertRow();
    void submitChanges();
    void revertChanges();

    // View operations
    void filterData();
    void sortData();
    void resetView();

private:
    void setupUI();
    void createMenus();
    void createToolBars();
    void loadSettings();
    void saveSettings();
    void refreshDatabaseList();  // Добавлено

    bool confirmAction(const QString &message);
    bool databaseExists(const QString &dbName);  // Добавлено
    QStringList getTableList();
    QStringList getDatabaseList() const;  // Добавлено
    QString currentTableName() const;
    void executeAndShowQuery(const QString &query);
    void showError(const QString &title, const QSqlError &error);

    QSqlTableModel *sqlModel;
    QTableView *tableView;
    QTextEdit *queryEditor;
    QStatusBar *statusBar;
    QDockWidget *queryDock;

    // Actions
    QAction *connectAction;
    QAction *disconnectAction;
    QAction *refreshAction;
    QAction *executeAction;
    QAction *showTablesAction;
    QAction *exportAction;
    QAction *importAction;
    QAction *copyAction;
    QAction *deleteAction;
    QAction *insertAction;
    QAction *submitAction;
    QAction *revertAction;
    QAction *filterAction;
    QAction *sortAction;
    QAction *resetAction;

    QSettings *settings;
    QString lastDir;
};

#endif // DATABASEADMIN_H
