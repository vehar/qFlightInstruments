#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QDebug>
#include <QHeaderView>
#include <QTableWidget>
#include <QtCore>
#include <QtGui>

#include "qFlightInstruments.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

QADI::QADI(QWidget *parent) : QWidget(parent)
{
    connect(this, SIGNAL(canvasReplot(void)), this, SLOT(canvasReplot_slot(void)));

    m_sizeMin = 200;
    m_sizeMax = 600;
    m_offset = 2;
    m_size = m_sizeMin - 2 * m_offset;

    setMinimumSize(m_sizeMin, m_sizeMin);
    setMaximumSize(m_sizeMax, m_sizeMax);
    resize(m_sizeMin, m_sizeMin);

    setFocusPolicy(Qt::NoFocus);

    m_roll = 0.0;
    m_pitch = 0.0;
}

QADI::~QADI() {}

void QADI::canvasReplot_slot(void) { update(); }

void QADI::resizeEvent(QResizeEvent *event) { m_size = qMin(width(), height()) - 2 * m_offset; }

void QADI::paintEvent(QPaintEvent *)
{
    const double PI = 3.1415926;
    const int HALF_CIRCLE = 180;
    const double FULL_CIRCLE = 360.0;
    const double HALF_SIZE = m_size / 2;
    const double SIZE_RATIO = HALF_SIZE / 45.0;
    const double PITCH_TEM_RATIO = HALF_SIZE * -m_pitch / 45.0;
    const int FONT_SIZE = 8;
    const int ROLL_LINES = 36;
    const double ROT_ANG = FULL_CIRCLE / ROLL_LINES;
    const int ROLL_LINE_LEN = m_size / 25;

    // Initialize pens, brushes, and QFont outside loops
    QPainter painter(this);

    QBrush bgSky(QColor(48, 172, 220));
    QBrush bgGround(QColor(247, 168, 21));

    QPen whitePen(Qt::white, 2);
    QPen blackPen(Qt::black, 2);
    QPen pitchPen(Qt::white, 2);
    QPen pitchZero(Qt::green, 3);

    QFont font("", FONT_SIZE);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);
    painter.rotate(m_roll);

    double pitch_tem = -m_pitch;

    // Cache reused computations
    double y_pitch = SIZE_RATIO * pitch_tem;
    int y = std::clamp(y_pitch, SIZE_RATIO * -40.0, SIZE_RATIO * 40.0);
    int x = sqrt(HALF_SIZE * HALF_SIZE - y * y);
    double gr = atan((double)(y) / x) * HALF_CIRCLE / PI;

    // draw background
    {
        painter.setPen(blackPen);
        painter.setBrush(bgSky);
        painter.drawChord(-HALF_SIZE, -HALF_SIZE, m_size, m_size, gr * 16,
                          (HALF_CIRCLE - 2 * gr) * 16);

        painter.setBrush(bgGround);
        painter.drawChord(-HALF_SIZE, -HALF_SIZE, m_size, m_size, gr * 16,
                          -(HALF_CIRCLE + 2 * gr) * 16);
    }

    QRegion maskRegion(-HALF_SIZE, -HALF_SIZE, m_size, m_size, QRegion::Ellipse);
    painter.setClipRegion(maskRegion);

    // draw pitch lines & marker
    {
        int ll = m_size / 8, l;
        QString s;
        s.reserve(4); // Pre-allocate string with enough memory

        painter.setFont(font);

        // draw lines
        for (int i = -9; i <= 9; i++)
        {
            int p = i * 10;

            s = QString::number(-p); // Prefer QString::number over arg

            l = (i % 3 == 0) ? ll : ll / 2;

            int y = SIZE_RATIO * p - PITCH_TEM_RATIO;
            int x = l;

            if (sqrt(x * x + y * y) > HALF_SIZE) // r
                continue;

            if (i == 0)
            {
                painter.setPen(pitchZero);
                l *= 1.8;
            }
            else
            {
                painter.setPen(pitchPen);
            }

            painter.drawLine(QPointF(-l, 1.0 * y), QPointF(l, 1.0 * y));

            if (i % 3 == 0 && i != 0)
            {
                painter.setPen(whitePen);
                int x1 = -x - 2 - 100;
                int y1 = y - FONT_SIZE / 2 - 1;
                painter.drawText(QRectF(x1, y1, 100, FONT_SIZE + 2),
                                 Qt::AlignRight | Qt::AlignVCenter, s);
            }
        }

        // draw marker
        int markerSize = m_size / 20;
        int halfMarkerSize = markerSize / 2;
        int doublemarkerSize = markerSize * 2;

        painter.setBrush(QBrush(Qt::red));
        painter.setPen(Qt::NoPen);

        constexpr int pointsNum = 3;
        QPointF points[pointsNum] = { QPointF(markerSize, 0),
                                      QPointF(doublemarkerSize, -halfMarkerSize),
                                      QPointF(doublemarkerSize, halfMarkerSize) };
        painter.drawPolygon(points, pointsNum);

        QPointF points2[pointsNum] = { QPointF(-markerSize, 0),
                                       QPointF(-doublemarkerSize, -halfMarkerSize),
                                       QPointF(-doublemarkerSize, halfMarkerSize) };
        painter.drawPolygon(points2, pointsNum);
    }

    // draw roll degree lines
    {
        painter.setPen(blackPen);
        painter.setFont(font);

        for (int i = 0; i < ROLL_LINES; i++)
        {
            QString s = (i < ROLL_LINES / 2) ? QString::number(-i * ROT_ANG)
                                             : QString::number(FULL_CIRCLE - i * ROT_ANG);

            double fy1 = -HALF_SIZE + m_offset;
            double fy2;

            if (i % 3 == 0)
            {
                fy2 = fy1 + ROLL_LINE_LEN;
                painter.drawLine(QPointF(0, fy1), QPointF(0, fy2));

                fy2 = fy1 + ROLL_LINE_LEN + 2;
                painter.drawText(QRectF(-50, fy2, 100, FONT_SIZE + 2), Qt::AlignCenter, s);
            }
            else
            {
                fy2 = fy1 + ROLL_LINE_LEN / 2;
                painter.drawLine(QPointF(0, fy1), QPointF(0, fy2));
            }

            painter.rotate(ROT_ANG);
        }
    }

    // draw roll marker
    {
        int rollMarkerSize = ROLL_LINE_LEN;
        int halfRollMarkerSize = rollMarkerSize / 2;

        painter.rotate(-m_roll);
        painter.setBrush(QBrush(Qt::black));

        QPointF points[3] = { QPointF(0, -HALF_SIZE + m_offset),
                              QPointF(-halfRollMarkerSize, -HALF_SIZE + m_offset + rollMarkerSize),
                              QPointF(halfRollMarkerSize, -HALF_SIZE + m_offset + rollMarkerSize) };
        painter.drawPolygon(points, 3);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

QCompass::QCompass(QWidget *parent) : QWidget(parent)
{
    connect(this, SIGNAL(canvasReplot(void)), this, SLOT(canvasReplot_slot(void)));

    m_sizeMin = 200;
    m_sizeMax = 600;
    m_offset = 2;
    m_size = m_sizeMin - 2 * m_offset;

    setMinimumSize(m_sizeMin, m_sizeMin);
    setMaximumSize(m_sizeMax, m_sizeMax);
    resize(m_sizeMin, m_sizeMin);

    setFocusPolicy(Qt::NoFocus);

    m_yaw = 0.0;
    m_alt = 0.0;
    m_h = 0.0;
}

QCompass::~QCompass() {}

void QCompass::canvasReplot_slot(void) { update(); }

void QCompass::resizeEvent(QResizeEvent *event) { m_size = qMin(width(), height()) - 2 * m_offset; }

void QCompass::paintEvent(QPaintEvent *)
{
    //    if (m_alt == m_previousAlt && m_h == m_previousH && m_yaw == m_previousYaw)
    //        return; // No data has changed, no need to repaint

    QPainter painter(this);

    QPen pen(Qt::white, 1);

    QFont smallFont("");
    smallFont.setPointSize(8);

    QFont bigFont(smallFont);
    bigFont.setPointSizeF(8 * 1.5);

    QFont boldFont("Helvetica", 14);
    boldFont.bold();

    int fontSizePlusTwo = QFontMetrics(smallFont).height() + 2;

    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2, height() / 2);

    // draw background
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.setBrush(QColor(48, 172, 220));
    painter.drawEllipse(-m_size / 2, -m_size / 2, m_size, m_size);

    // draw yaw lines
    QPointF lineStart(0, -m_size / 2 + m_offset);
    QPointF arrowPoints[3];
    pen.setWidth(1);
    painter.setPen(pen);

    for (int i = 0; i < 36; ++i)
    {
        QColor color;
        if (i == 0)
        {
            color = Qt::blue;
            painter.setFont(bigFont);
        }
        else if (i == 9 || i == 27)
        {
            color = Qt::black;
            painter.setFont(bigFont);
        }
        else if (i == 18)
        {
            color = Qt::red;
            painter.setFont(bigFont);
        }
        else
        {
            color = Qt::black;
            painter.setFont(smallFont);
        }

        if (pen.color() != color)
        {
            pen.setColor(color);
            painter.setPen(pen);
        }

        if (i % 3 == 0)
        {
            QPointF lineEnd(0, lineStart.y() + m_size / 25);
            painter.drawLine(lineStart, lineEnd);
            painter.drawText(QRectF(-50, lineEnd.y() + 4, 100, fontSizePlusTwo), Qt::AlignCenter,
                             QString::number(i * 10));
        }
        else
        {
            painter.drawLine(lineStart, QPointF(0, lineStart.y() + m_size / 50));
        }

        painter.rotate(-10);
    }

    // draw S/N arrow
    int arrowSize = 10;
    painter.setPen(Qt::NoPen);
    arrowPoints[0] = QPointF(0, -m_size / 2 + m_offset + m_size / 25 + 15);
    arrowPoints[1] = QPointF(-m_size / arrowSize, 0);
    arrowPoints[2] = QPointF(m_size / arrowSize, 0);

    painter.setBrush(QColor(0x00, 0x00, 0xFF, 0xA0)); // Blue
    painter.drawPolygon(arrowPoints, 3);

    arrowPoints[0].setY(m_size / 2 - m_offset - m_size / 25 - 15);
    painter.setBrush(QColor(0xFF, 0x00, 0x00, 0xA0)); // Red
    painter.drawPolygon(arrowPoints, 3);

    // draw yaw marker
    int markerSize = 24;
    painter.rotate(-m_yaw);
    painter.setBrush(QColor(0xFF, 0x00, 0x00, 0xE0)); // Red
    arrowPoints[0] = QPointF(0, -m_size / 2 + m_offset);
    arrowPoints[1] = QPointF(-m_size / markerSize, arrowPoints[0].y() + m_size / (markerSize / 2));
    arrowPoints[2] = QPointF(m_size / markerSize, arrowPoints[1].y());
    painter.drawPolygon(arrowPoints, 3);
    painter.rotate(m_yaw);

#ifdef DRAW_ALTITUDE
    // draw altitude
    pen.setWidth(2);
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.setBrush(QColor(128, 128, 128, 240));
    painter.setFont(QFont("", 13));
    QRect altRect(-65, -13 - 8, 130, 2 * (13 + 8));
    painter.drawRoundedRect(altRect, 6, 6);

    pen.setColor(Qt::green);
    painter.setPen(pen);
    painter.setFont(boldFont);
    painter.drawText(altRect.adjusted(0, 2, 0, -altRect.height() / 2), Qt::AlignCenter,
                     QString::asprintf("ALT: %6.1f m", m_alt));
    painter.drawText(altRect.adjusted(0, altRect.height() / 2, 0, 0), Qt::AlignCenter,
                     QString::asprintf("H: %6.1f m", m_h));
#endif
    //    m_previousAlt = m_alt;
    //    m_previousH = m_h;
    //    m_previousYaw = m_yaw;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

QKeyValueListView::QKeyValueListView(QWidget *parent) : QTableWidget(parent)
{
    connect(this, SIGNAL(listUpdate(void)), this, SLOT(listUpdate_slot(void)));

    m_mutex = new QMutex();

    // set row & column numbers
    setRowCount(0);
    setColumnCount(2);

    // set no headers
    // verticalHeader()->hide();
    // horizontalHeader()->hide();
    QStringList htb = { "Name", "Value" };
    this->setHorizontalHeaderLabels(htb);

    // set last section is stretch-able
    QHeaderView *HorzHdr = horizontalHeader();
    HorzHdr->setStretchLastSection(true);
    HorzHdr->resizeSection(0, 80); // set first column width

    // disable table edit & focus
    setEditTriggers(QTableWidget::NoEditTriggers);
    setFocusPolicy(Qt::NoFocus);
    setMinimumHeight(10);
}

QKeyValueListView::~QKeyValueListView() { delete m_mutex; }

void QKeyValueListView::createOrUpdateItem(int row, int column, const QString &text,
                                           const QColor &foreground, const QColor &background)
{
    const int fontSize = 10;

    QTableWidgetItem *item = this->item(row, column);
    if (item != nullptr)
    {
        item->setText(text);
        return;
    }

    item = new QTableWidgetItem(text);
    item->setForeground(foreground);
    item->setBackground(background);
    item->setFont(QFont("", fontSize));
    this->setItem(row, column, item);
}

void QKeyValueListView::listUpdate_slot(void)
{
    QColor clCL1(0x00, 0x00, 0xFF);
    QColor clCL2(0x00, 0x00, 0x00);
    QColor clB1(0xF0, 0xF0, 0xF0);
    QColor clB2(0xE0, 0xE0, 0xE0);

    const int rowHeight = 10;

    m_mutex->lock();
    setRowCount(m_data.size());

    int i = 0;
    for (auto it = m_data.begin(); it != m_data.end(); i++, it++)
    {
        QColor background = (i % 2 == 0) ? clB1 : clB2;
        createOrUpdateItem(i, 0, it.key(), clCL1, background);   // set name cell
        createOrUpdateItem(i, 1, it.value(), clCL2, background); // set value cell
        setRowHeight(i, rowHeight);
    }

    m_mutex->unlock();
}
