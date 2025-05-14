#ifndef DATABASEADMIN_H
#define DATABASEADMIN_H

#include <QMainWindow>
#include <QSqlQueryModel>
#include <QSettings>
#include <QItemSelection>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QStatusBar;
class QDockWidget;
class QMenu;
class QToolBar;
class QAction;
class QSqlDatabase;
class QSqlQuery;
class QStandardItemModel;
QT_END_NAMESPACE

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
    void connectToDatabase();
    void disconnectFromDatabase();
    void showDatabaseInfo();
    void refreshData();

    // Table operations
    void showTables();
    void showTableStructure();
    void createTable();
    void dropTable();
    void renameTable();
    void truncateTable();

    // Data operations
    void executeQuery();
    void exportToCSV();
    void importFromCSV();
    void copyData();
    void pasteData();
    void deleteSelectedRows();
    void insertRow();

    // View operations
    void filterData();
    void sortData();
    void resetView();

    // Settings
    void showPreferences();
    void about();

private:
    void setupUI();
    void setupConnections();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void loadSettings();
    void saveSettings();
    bool confirmAction(const QString &message);

    // Database helpers
    bool checkDatabaseConnection();
    QStringList getTableList();
    QString currentTableName() const;

    // CSV helpers
    QStringList parseCSVLine(const QString &line);
    QString escapeCSVValue(const QString &value) const;

    // Data helpers
    void updateRowCount();
    void executeAndShowQuery(const QString &query);
    void showError(const QString &title, const QSqlError &error);

    // UI components
    QTableView *tableView;
    QTextEdit *queryEditor;
    QStatusBar *statusBar;
    QDockWidget *queryDock;

    // Models
    QSqlQueryModel *sqlModel;
    QStandardItemModel *csvModel;

    // Actions
    QAction *connectAction;
    QAction *disconnectAction;
    QAction *refreshAction;
    QAction *executeAction;
    QAction *showTablesAction;
    QAction *exportAction;
    QAction *importAction;
    QAction *copyAction;
    QAction *pasteAction;
    QAction *deleteAction;
    QAction *insertAction;
    QAction *filterAction;
    QAction *sortAction;
    QAction *resetAction;

    // Settings
    QSettings *settings;
    QString lastDir;
    bool autoRefresh;
    int refreshInterval;
    QString lastDatabase;
};

#endif // DATABASEADMIN_H
