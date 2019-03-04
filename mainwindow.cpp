#include "mainwindow.h"
#include "ui_mainwindow.h"

const char flexoLookupLower[64] =
{
    (char)0x90, (char)0x90, 'e', '8', (char)0x90, '|', 'a', '3', ' ', '=', 's', '4',
    'i', '+', 'u', '2', (char)0x91, '.', 'd', '5', 'r', '1', 'j', '7', 'n', ',', 'f', '6', 'c', '-', 'k', (char)0x90,
    't', (char)0x90, 'z', '\b', 'l', '\t', 'w', (char)0x90, 'h', '\r', 'y', (char)0x90, 'p', (char)0x90, 'q', (char)0x90, 'o',
    '\0', 'b', (char)0x90, 'g', (char)0x90, '9', (char)0x90, 'm', (char)0x92, 'x', (char)0x90, 'v', (char)0x93, '0', (char)0x7F
};

const char flexoLookupUpper[64] =
{
    (char)0x90, (char)0x90, 'E', '8', (char)0x90, '_', 'A', '3', ' ', ':', 'S', '4',
    'I', '/', 'U', '2', (char)0x91, ')', 'D', '5', 'R', '1', 'J', '7', 'N', '(', 'F', '6', 'C', '~', 'K', (char)0x90,
    'T', (char)0x90, 'Z', '\b', 'L', '\t', 'W', (char)0x90, 'H', '\r', 'y', (char)0x90, 'P', (char)0x90, 'Q', (char)0x90, 'O',
    '\0', 'B', (char)0x90, 'G', (char)0x90, '9', (char)0x90, 'M', (char)0x92, 'X', (char)0x90, 'V', (char)0x93, '0', (char)0x7F
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->startStop, SIGNAL(clicked(bool)), this, SLOT(onStartStop()));
    connect(ui->restart, SIGNAL(clicked(bool)), this, SLOT(onReset()));
    connect(ui->step, SIGNAL(clicked(bool)), this, SLOT(onStep()));
    connect(ui->poke, SIGNAL(clicked(bool)), this, SLOT(onPoke()));
    connect(ui->peek, SIGNAL(clicked(bool)), this, SLOT(onPeek()));
    connect(ui->memLoad, SIGNAL(clicked(bool)), this, SLOT(onMemLoad()));
    connect(ui->memSave, SIGNAL(clicked(bool)), this, SLOT(onMemSave()));
    connect(ui->tapeLoad, SIGNAL(clicked(bool)), this, SLOT(onTapeLoad()));
    connect(ui->tapeSave, SIGNAL(clicked(bool)), this, SLOT(onTapeSave()));
    connect(ui->mode, SIGNAL(clicked(bool)), this, SLOT(onMode()));

    systimer = new QTimer(this);
    connect(systimer, SIGNAL(timeout()), this, SLOT(onTick()));
    systimer->setInterval(2);

    tube = new QPixmap(512, 512);
    tubeScene = new QGraphicsScene(this);
    tubeCursor = new QPainter(tube);

    tubePixRef = tubeScene->addPixmap(*tube);
    tubeScene->setSceneRect(tube->rect());
    ui->vector->setScene(tubeScene);
    tubeCursor->setPen(*(new QColor(111, 249, 107)));

    memset(core, 0, sizeof(uint32_t)*65536);
    reset();

    updateMemory();
    updateRegisters();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reset()
{
    mbr = 0;
    ac = 0;
    lr = 0;
    tbr = 0;
    tac = 0;
    mar = 0;
    pc = 0;
    ir = 0;

    halt = false;
    flexColor = false;
    flexUpper = false;
    advancedMode = false;

    memCurrent = 0;

    tube->fill(QColor(20, 20, 20));
    tubePixRef->setPixmap(*tube);
}

void MainWindow::execute()
{
    if(halt)
        return;

    ir = (core[pc] & 0x30000) >> 16;
    mar = (core[pc] & 0x0000FFFF);

    switch(ir)
    {
        case(0):
        {
            // store
            core[mar] = ac;
        }
        break;

        case(1):
        {
            // add
            ac += core[mar];
        }
        break;

        case(2):
        {
            // branch
            if((ac & 0x20000) > 0)
                pc = mar-1;
        }
        break;

        case(3):
        {
            // operate - certified kinda skookum

            if((mar & 0x8000) == 0x8000) { ac &= 0x001FF; } // cll
            if((mar & 0x4000) == 0x4000) { ac &= 0x3FE00; } // clr

            if((mar & 0x3000) == 0x2000) {} // ios
            if((mar & 0x3000) == 0x3000) { halt = false; } // hlt

            if((mar & 0x0E00) == 0x0E00)
            {
                uint8_t c = 0;

                c |= ((ac & 0x08000) > 0) ? 0x20 : 0;
                c |= ((ac & 0x01000) > 0) ? 0x10 : 0;
                c |= ((ac & 0x00200) > 0) ? 0x08 : 0;
                c |= ((ac & 0x00040) > 0) ? 0x04 : 0;
                c |= ((ac & 0x00008) > 0) ? 0x02 : 0;
                c |= ((ac & 0x00001) > 0) ? 0x01 : 0;

                c <<= 1;
                c |= 1;

                punchTape(c);
            } // p7h
            if((mar & 0x0E00) == 0x0C00)
            {
                uint8_t c = 0;

                c |= ((ac & 0x08000) > 0) ? 0x20 : 0;
                c |= ((ac & 0x01000) > 0) ? 0x10 : 0;
                c |= ((ac & 0x00200) > 0) ? 0x08 : 0;
                c |= ((ac & 0x00040) > 0) ? 0x04 : 0;
                c |= ((ac & 0x00008) > 0) ? 0x02 : 0;
                c |= ((ac & 0x00001) > 0) ? 0x01 : 0;

                c <<= 1;

                punchTape(c);
            } // p6h
            if((mar & 0x0E00) == 0x0800)
            {
                uint8_t c = 0;

                c |= ((ac & 0x08000) > 0) ? 0x20 : 0;
                c |= ((ac & 0x01000) > 0) ? 0x10 : 0;
                c |= ((ac & 0x00200) > 0) ? 0x08 : 0;
                c |= ((ac & 0x00040) > 0) ? 0x04 : 0;
                c |= ((ac & 0x00008) > 0) ? 0x02 : 0;
                c |= ((ac & 0x00001) > 0) ? 0x01 : 0;

                printFlexo(c);

            } // pnt
            if((mar & 0x0E00) == 0x0200)
            {
                uint32_t c = (uint32_t)readTape();
                c >>= 1;
                qInfo() << c << "\n";
                ac &= ~(0x24924);
                ac |= ((c & 0x20) >> 5) << 17;
                ac |= ((c & 0x10) >> 4) << 14;
                ac |= ((c & 0x08) >> 3) << 11;
                ac |= ((c & 0x04) >> 2) << 8;
                ac |= ((c & 0x02) >> 1) << 5;
                ac |= ((c & 0x01) >> 0) << 2;
            } // r1c
            if((mar & 0x0E00) == 0x0600)
            {
                uint32_t c = (uint32_t)readTape();
                c >>= 1;

                ac &= ~0x24924;
                ac |= (c & 0x20 >> 5) << 17;
                ac |= (c & 0x10 >> 4) << 14;
                ac |= (c & 0x08 >> 3) << 11;
                ac |= (c & 0x04 >> 2) << 8;
                ac |= (c & 0x02 >> 1) << 5;
                ac |= (c & 0x01 >> 0) << 2;

                if((ac & 0x1) > 0) {ac |= 0x40000;} ac >>= 1; ac &= 0x3FFFF;

                c = (uint32_t)readTape();
                c >>= 1;

                ac &= ~0x24924;
                ac |= (c & 0x20 >> 5) << 17;
                ac |= (c & 0x10 >> 4) << 14;
                ac |= (c & 0x08 >> 3) << 11;
                ac |= (c & 0x04 >> 2) << 8;
                ac |= (c & 0x02 >> 1) << 5;
                ac |= (c & 0x01 >> 0) << 2;

                if((ac & 0x1) > 0) {ac |= 0x40000;} ac >>= 1; ac &= 0x3FFFF;

                c = (uint32_t)readTape();
                c >>= 1;

                ac &= ~0x24924;
                ac |= (c & 0x20 >> 5) << 17;
                ac |= (c & 0x10 >> 4) << 14;
                ac |= (c & 0x08 >> 3) << 11;
                ac |= (c & 0x04 >> 2) << 8;
                ac |= (c & 0x02 >> 1) << 5;
                ac |= (c & 0x01 >> 0) << 2;

                if((ac & 0x1) > 0) {ac |= 0x40000;} ac >>= 1; ac &= 0x3FFFF;

            } // r3c
            if((mar & 0x0E00) == 0x0400)
            {
                int xp = (ac & 0x1FE00) >> 10;
                int yp = (ac & 0x000FF) >> 0;

                xp = ((ac & 0x20000) > 0) ? ((~xp) & 0xFF) * -1 : xp;
                yp = ((ac & 0x00100) > 0) ? ((~yp) & 0xFF) * -1 : yp;

                tubeCursor->drawRect(xp+256, yp+256, 1, 1);
                tubePixRef->setPixmap(*tube);
            } // dis

            if((mar & 0x0180) == 0x0100) { ac >>= 1; ac &= 0x3FFFF; } // shr
            if((mar & 0x0180) == 0x0180) { if((ac & 0x1) > 0) ac |= 0x40000; ac >>= 1; ac &= 0x3FFFF; } // cyr
            if((mar & 0x0180) == 0x0080) { lr = mbr; } // mlr

            if((mar & 0x0044) == 0x0040) {} // pen
            if((mar & 0x0044) == 0x0004) { ac |= tac; } // tac

            if((mar & 0x0020) == 0x0020) { ac = ~ac; } // com
            if((mar & 0x0010) == 0x0010) { ac = ac ^ mbr; } // pad
            if((mar & 0x0008) == 0x0008)
            {
                int carry = ac ^ mbr;
                carry <<= 1; if((carry & 0x40000) > 0) {carry |= 1; } carry &= 0x3FFFF;
                ac = carry ^ ac;
            } // cry

            if((mar & 0x0003) == 0x0001) { mbr = ac; } // amb
            if((mar & 0x0003) == 0x0003) { mbr = tbr; } // tbr
            if((mar & 0x0003) == 0x0002) { mbr = lr; } // lmb
        }
        break;

        default: break;
    }

    pc++;
}

void MainWindow::updateRegisters()
{
    // I am embarassed
    QString lm = QString(QString((!advancedMode) ? "1958 mode" : "1961 mode") + "\n");

    QString l1 = QString("pc          " + QString("%1").arg(pc, 16, 2, QChar('0')).right(16) + "  0x" + QString("%1").arg(pc, 4, 16, QChar('0')).right(4) + "\n");
    QString l2 = QString("ir:mar    " + QString("%1").arg((ir << 16) | mar, 18, 2, QChar('0')).right(18) + " 0x" + QString("%1").arg((ir << 16) | mar, 5, 16, QChar('0')).right(5) + "\n");
    QString l3 = QString("ac        " + QString("%1").arg(ac, 18, 2, QChar('0')).right(18) + " 0x" + QString("%1").arg(ac, 5, 16, QChar('0')).right(5) + "\n");
    QString l4 = QString("lr        " + QString("%1").arg(lr, 18, 2, QChar('0')).right(18) + " 0x" + QString("%1").arg(lr, 5, 16, QChar('0')).right(5) + "\n");
    QString l5 = QString("mbr       " + QString("%1").arg(mbr, 18, 2, QChar('0')).right(18) + " 0x" + QString("%1").arg(mbr, 5, 16, QChar('0')).right(5));

    ui->regView->setText(QString(lm+l1+l2+l3+l4+l5));
}

void MainWindow::updateMemory()
{
    ui->memView->clear();

    for(long int i=0;i<16*8;i+=8)
    {
        ui->memView->appendPlainText(QString("0x" + QString("%1").arg(memCurrent+i, 4, 16, QChar('0'))
                                           + ": " + QString("%1").arg(core[memCurrent+i], 5, 16, QChar('0'))
                                           + " " + QString("%1").arg(core[memCurrent+i+1], 5, 16, QChar('0'))
                                           + " " + QString("%1").arg(core[memCurrent+i+2], 5, 16, QChar('0'))
                                           + " " + QString("%1").arg(core[memCurrent+i+3], 5, 16, QChar('0'))
                                           + " " + QString("%1").arg(core[memCurrent+i+4], 5, 16, QChar('0'))
                                           + " " + QString("%1").arg(core[memCurrent+i+5], 5, 16, QChar('0'))
                                           + " " + QString("%1").arg(core[memCurrent+i+6], 5, 16, QChar('0'))
                                           + " " + QString("%1").arg(core[memCurrent+i+7], 5, 16, QChar('0')) ));
    }

    QScrollBar *vScrollBar = ui->memView->verticalScrollBar();
    vScrollBar->triggerAction(QScrollBar::SliderToMinimum);
}

void MainWindow::printFlexo(uint8_t c)
{
    uint8_t work = (flexUpper) ? flexoLookupUpper[c] : flexoLookupLower[c];

    switch(work)
    {
        case(0x90): break;
        case(0x91): flexColor = (flexColor) ? false : true; break;
        case(0x92): flexUpper = true; break;
        case(0x93): flexUpper = false; break;
        default:
        {
            ui->flexo->moveCursor(QTextCursor::End);
            ui->flexo->insertPlainText(QString(work));
            ui->flexo->moveCursor(QTextCursor::End);
        }
        break;
    }
}

void MainWindow::startTape()
{
    if(currentTape.size() == 0) currentTape.append("    #     ");

    tapePointer = 0;
}

void MainWindow::punchTape(uint8_t w)
{
    QString t = "    #     ";

    if((w & 0x40) > 0) t[1] = 'O';
    if((w & 0x20) > 0) t[2] = 'O';
    if((w & 0x10) > 0) t[3] = 'O';
    if((w & 0x08) > 0) t[5] = 'O';
    if((w & 0x04) > 0) t[6] = 'O';
    if((w & 0x02) > 0) t[7] = 'O';
    if((w & 0x01) > 0) t[8] = 'O';

    currentTape[tapePointer] = t;

    tapePointer++;
    if(tapePointer >= currentTape.size()) tapePointer--;
}

uint8_t MainWindow::readTape()
{
    uint8_t v = 0;
    QString t = currentTape[tapePointer];
    qInfo() << t << "\n";

    v |= ((t[1] == 'O') ? 1 : 0) << 6;
    v |= ((t[2] == 'O') ? 1 : 0) << 5;
    v |= ((t[3] == 'O') ? 1 : 0) << 4;
    v |= ((t[5] == 'O') ? 1 : 0) << 3;
    v |= ((t[6] == 'O') ? 1 : 0) << 2;
    v |= ((t[7] == 'O') ? 1 : 0) << 1;
    v |= ((t[8] == 'O') ? 1 : 0) << 0;

    tapePointer++;
    if(tapePointer >= currentTape.size()) tapePointer--;

    return v;
}

void MainWindow::updateTape()
{
    ui->tape->clear();

    for(int i=0;i<currentTape.size();i++)
    {
        QString l = currentTape[i];
        if(i == tapePointer) { l[0] = '>'; l[9] = '<'; }
        ui->tape->append(l);
    }

    QScrollBar *vScrollBar = ui->tape->verticalScrollBar();
    vScrollBar->triggerAction(QScrollBar::SliderToMinimum);
}

void MainWindow::onStep()
{
    halt = false;
    execute();
    updateRegisters();
    updateTape();
}

void MainWindow::onStartStop()
{
    if(halt)
    {
        halt = false;
        ui->startStop->setText("STOP");
        systimer->start();
    }
    else
    {
        halt = true;
        ui->startStop->setText("START");
        systimer->stop();
        updateRegisters();
        updateTape();
    }
}

void MainWindow::onReset()
{
    reset();
    updateRegisters();
    startTape();
    updateTape();
    ui->flexo->clear();
}

void MainWindow::onTick()
{
    while(!halt)
    {
        QCoreApplication::processEvents();
        execute();
    }
}

void MainWindow::onMode()
{
    advancedMode = (advancedMode) ? false : true;
    updateRegisters();
}

void MainWindow::onPoke()
{
    int loc = ui->pokeAddr->text().toUInt(NULL, 16);
    int val = ui->pokeData->text().toUInt(NULL, 16);

    core[loc] = val;

    updateMemory();
}

void MainWindow::onPeek()
{
    memCurrent = ui->pokeAddr->text().toUInt(NULL, 16);

    updateMemory();
}

void MainWindow::onMemLoad()
{
    QFile memDump(QFileDialog::getOpenFileName(this, "Open Core", "/home", "Binary Files (*.bin)"));
    if (!memDump.open(QIODevice::ReadOnly)) return;

    QByteArray loadedFile = memDump.readAll();
    if((loadedFile.size() % 3) != 0) return;
    if((loadedFile.size() / 3) > 65536) return;

    for(int m=0;m<loadedFile.size();m+=3)
    {
        uint32_t word = 0;
        word |= (loadedFile[m] << 16);
        word |= (loadedFile[m+1] << 8);
        word |= (loadedFile[m+2]);
        core[m/3] = word;
    }

    memDump.close();
    updateMemory();
}

void MainWindow::onMemSave()
{
    QFile memDump(QFileDialog::getSaveFileName(this, "Save Core", "/home", "Binary Files (*.bin)"));
    if (!memDump.open(QIODevice::WriteOnly)) return;

    QByteArray coreSave;

    for(int m=0;m<65536;m++)
    {
        coreSave.append(((core[m] & 0xFF0000) >> 16) & 0xFF);
        coreSave.append(((core[m] & 0xFF00) >> 8) & 0xFF);
        coreSave.append(((core[m] & 0xFF)) & 0xFF);
    }

    memDump.write(coreSave, coreSave.size());

    memDump.close();
    updateMemory();
}

void MainWindow::onTapeLoad()
{
    QFile tapeDump(QFileDialog::getOpenFileName(this, "Load Tape", "/home", "Tape Files (*.txt)"));
    if (!tapeDump.open(QIODevice::ReadOnly)) return;

    currentTape.clear();
    QTextStream tapeStream(&tapeDump);

    while(!tapeStream.atEnd())
    {
        currentTape.append(tapeDump.readLine().split('\n')[0]);
    }

    tapeDump.close();
    updateTape();
}

void MainWindow::onTapeSave()
{
    QFile tapeDump(QFileDialog::getSaveFileName(this, "Save Tape", "/home", "Tape Files (*.txt)"));
    if (!tapeDump.open(QIODevice::WriteOnly)) return;

    QTextStream tapeStream(&tapeDump);

    for(int m=0;m<currentTape.size();m++)
    {
        tapeStream << currentTape[m] << '\n';
    }

    tapeDump.close();
    updateTape();
}

