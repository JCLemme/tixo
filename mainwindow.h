#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QScrollBar>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QFileDialog>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void reset();
    void execute();
    void updateRegisters();
    void updateMemory();

    void printFlexo(uint8_t c);

    void startTape();
    void punchTape(uint8_t w);
    uint8_t readTape();
    void updateTape();

public slots:
    void onStep();
    void onStartStop();
    void onReset();
    void onTick();

    void onMode();

    void onPoke();
    void onPeek();

    void onMemLoad();
    void onMemSave();
    void onTapeLoad();
    void onTapeSave();

private:
    Ui::MainWindow *ui;

    uint32_t core[65536];
    uint32_t mbr, ac, lr, tbr, tac;
    uint16_t mar, pc;
    uint8_t ir;

    QTimer* systimer;
    QPixmap* tube;
    QGraphicsScene* tubeScene;
    QPainter* tubeCursor;
    QGraphicsPixmapItem* tubePixRef;

    uint32_t memCurrent;
    int tapePointer;
    QList<QString> currentTape;
    bool halt, flexColor, flexUpper, advancedMode;
};

#endif // MAINWINDOW_H
