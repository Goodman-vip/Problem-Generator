#pragma once

#include <QtWidgets/QMainWindow>
#include <QFile>
#include <QStandardPaths>
#include <QGraphicsDropShadowEffect>
#include <QMoveEvent>
#include <QWheelEvent>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QtPrintSupport>
#include <QPagedPaintDevice>
#include <random>
#include "ui_ProblemGenerator.h"
#include "UTF_8.h"

class ProblemGenerator : public QMainWindow
{
    Q_OBJECT

public:
    ProblemGenerator(QWidget *parent = nullptr);
    ~ProblemGenerator();

private slots:
    void on_saveTxtBtn_clicked();
    void on_viewPdfBtn_clicked();
    void on_savePdfBtn_clicked();
    void on_generatorBtn_clicked();
    void on_saveBtn_clicked();

protected:
    int rand(int l,int r) {
        if (l > r) std::swap(l, r); 
        std::uniform_int_distribution<int> dist(l, r);
        return dist(global_urng());
    }
    void Connect_Init();
    void UI_Init();
    void getNums();
    void showMsg(QString msg);
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            mouse_press = true;
            mousePoint = event->globalPosition().toPoint() - this->pos();
        }
        else if (event->button() == Qt::RightButton) {
            if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
                this->close();
            }
        }
    }
    void mouseMoveEvent(QMouseEvent* event) override {
        if (mouse_press) {
            move(event->globalPosition().toPoint() - mousePoint);
        }
    }
    void mouseReleaseEvent(QMouseEvent* event) override { mouse_press = false; }
    void wheelEvent(QWheelEvent* event)  override {
        QPoint numPixels = event->pixelDelta();
        QPoint numDegrees = event->angleDelta();
        if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
            if (numDegrees.y() - numPixels.y() > 0) {
                m_rec = m_rec + 0.1;
                if (m_rec > 1.0)  m_rec = 1.0;
                setWindowOpacity(m_rec);
            }
            if (numDegrees.y() - numPixels.y() < 0) {
                m_rec = m_rec - 0.1;
                if (m_rec < 0.05)  m_rec = 0.05;
                setWindowOpacity(m_rec);
            }
        }
        else  QWidget::wheelEvent(event);
    }
    void drawQuestions(QPagedPaintDevice* device, int col, int num)
    {
        if (col <= 0 || num <= 0) return;

        QPainter painter(device);
        if (!painter.isActive()) return;

        if (!portraitRtn && num > 20)  num = 20;
        if (portraitRtn && num > 30)  num = 30;

        int dpi = device->logicalDpiX(); 
        QRectF paintRect = device->pageLayout().paintRectPixels(dpi);
        qreal pageWidth = paintRect.width();
        qreal pageHeight = paintRect.height();

        // 边距
        const qreal marginLeft = marginNum;
        const qreal marginTop = 200;
        const qreal marginRight = 0;
        const qreal marginBottom = 50;

        qreal contentWidth = pageWidth - marginLeft - marginRight;
        qreal contentHeight = pageHeight - marginTop - marginBottom;

        if (contentWidth <= 0 || contentHeight <= 0) return;

        qreal colWidth = contentWidth / static_cast<qreal>(col);
        qreal rowHeight = contentHeight / static_cast<qreal>(num);

        // 设置字体
        QFont font("Microsoft YaHei", 12);
        if (!QFontInfo(font).fixedPitch()) {
            font = QFont("Arial", 12);
        }
        int pre = 0;
        painter.setFont(font);
        painter.setPen(Qt::black);

        go:QRectF informationRect(0, 50, pageWidth - 100, 150);
        painter.drawText(informationRect, Qt::AlignRight | Qt::AlignVCenter, u8"班级  __________    姓名  __________    时间  __________    成绩  __________");

        // 绘制题目
        for (int c = 0; c < col; ++c) {
            for (int r = 0; r < num; ++r) {
                int questionIndex = pre + c * num + r + 1;
                if (questionIndex > total)  break;

                qreal x = marginLeft + c * colWidth;
                qreal y = marginTop + r * rowHeight;
                QRectF borderRect(x, y, colWidth, rowHeight);
                QRectF textRect(x + 10, y + 5, colWidth - 10, rowHeight - 20);
                QString text = questions[questionIndex - 1];
                if (ansRtn)  text += ans[questionIndex - 1];

                painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
                if(borderRtn)  painter.drawRect(borderRect); // 绘制边界框
            }
        }
        if (pre + col * num < total) {
            pre += col * num;
            device->newPage();
            goto go;
        }
    }
    void generatePdfOrPrint(int col, int num, int type)
    {
        if (col <= 0 || num <= 0) {
            QMessageBox::warning(nullptr, QString(u8"参数错误"),
                QString(u8"列数和每列题数必须大于0"));
            return;
        }
        if (questions.isEmpty()) { showMsg(QString(u8"题目集为空，请确认是否已点击生成键")); return; }

        const int targetDpi = 300; // 分辨率

        switch (type) {
        case 0: { // 预览
            QPrinter printer(QPrinter::HighResolution);
            printer.setResolution(targetDpi);
            printer.setPageSize(QPageSize::A4);
            if(portraitRtn)  printer.setPageOrientation(QPageLayout::Portrait);
            else  printer.setPageOrientation(QPageLayout::Landscape);

            QPrintPreviewDialog preview(&printer);
            QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                [&](QPrinter* p) {
                    drawQuestions(p, col, num);
                });
            preview.exec();
            break;
        }
        case 1: { // 导出 PDF
            QString fileName = QFileDialog::getSaveFileName(
                nullptr,
                QString(u8"保存PDF"),
                "Problems.pdf",
                "PDF Files (*.pdf)"
            );
            if (fileName.isEmpty()) return;
            if (!fileName.endsWith(".pdf", Qt::CaseInsensitive))
                fileName += ".pdf";

            QPdfWriter pdfWriter(fileName);
            pdfWriter.setResolution(targetDpi); 
            pdfWriter.setPageSize(QPageSize::A4);
            if (portraitRtn)  pdfWriter.setPageOrientation(QPageLayout::Portrait);
            else  pdfWriter.setPageOrientation(QPageLayout::Landscape);

            drawQuestions(&pdfWriter, col, num);

            QMessageBox::information(nullptr, QString(u8"成功"),
                QString(u8"PDF 已保存到:\n") + fileName);
            break;
        }
        case 2: { // 直接打印
            QPrinter printer(QPrinter::HighResolution);
            printer.setResolution(targetDpi);
            printer.setPageSize(QPageSize::A4);
            if (portraitRtn)  printer.setPageOrientation(QPageLayout::Portrait);
            else  printer.setPageOrientation(QPageLayout::Landscape);

            QPrintDialog printDialog(&printer);
            if (printDialog.exec() == QDialog::Accepted) {
                drawQuestions(&printer, col, num);
            }
            break;
        }

        default:
            QMessageBox::warning(nullptr, QString(u8"错误"),
                QString(u8"无效的 type 参数（0=预览, 1=PDF, 2=打印）"));
            break;
        }
    }
    void loadConfigFromCache()
    {
        QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QString filePath = cachePath + "/data.txt";

        QFile file(filePath);
        if (!file.exists()) return;

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open config file for reading:" << filePath;
            return;
        }

        int now_edition;
        QTextStream in(&file);
		in >> now_edition;
        if (now_edition != edition) {
            file.close();
            return;
		}
        in >> maxNum >> minAns >> maxAns >> colNum >> marginNum;

        QString rest = in.readAll().trimmed();
        file.close();

        QStringList parts = rest.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 10) {
            qWarning() << "Config file format invalid.";
            return;
        }

        segNum = parts[0];
        totNum = parts[1];
        posRtn = parts[2].toInt();
        borderRtn = parts[3].toInt();
        portraitRtn = parts[4].toInt();
        ansRtn = parts[5].toInt();

        addRtn = parts[6].toInt();
		minusRtn = parts[7].toInt();
		timesRtn = parts[8].toInt();
        divideRtn = parts[9].toInt();

        ui.get_maxNum->setText(QString::number(maxNum));
        ui.get_minAns->setText(QString::number(minAns));
        ui.get_maxAns->setText(QString::number(maxAns));
        ui.get_colNum->setText(QString::number(colNum));
		ui.get_marginNum->setText(QString::number(marginNum));
        ui.get_segNum->setText(segNum); 
        ui.get_totNum->setText(totNum);
        ui.posRtn->setChecked(posRtn);
        ui.borderRtn->setChecked(borderRtn);
        ui.portraitRtn->setChecked(portraitRtn);
        ui.ansRtn->setChecked(ansRtn);
        ui.addRtn->setChecked(addRtn);
		ui.minusRtn->setChecked(minusRtn);
		ui.timesRtn->setChecked(timesRtn);
		ui.divideRtn->setChecked(divideRtn);
    }
    /*QString generateQuestion(int seg)
    {
        int now = 0, cnt = 0; QString s;
        go:now=rand(1, maxNum),s = QString("%1").arg(now);
        for (int i = 1, f; i <= seg; i++) {
            if (rand(0, 1))  s += "+", f = 1;
            else  s += "-", f = -1;
            int num = rand(1, maxNum);
            s += QString("%1").arg(num);
            if (posRtn && now+num*f < 0){
                if(cnt>1000000)  return "";
                cnt++; goto go;
            }
            now += num * f;
        }
        if (now<minAns || now>maxAns) {
            if (cnt > 1000000)  return "";
            cnt++; goto go;
        }
        s += "=";
        ans.emplace_back(QString::number(now));
        questions.emplace_back(s);
        return s;
    }*/
    int countOperators(const QString& s) {
        int cnt = 0;
        for (QChar c : s)  if ((c <'0' || c>'9') && c!='=')  cnt++;
        return cnt;
    }
    QString generateSegment(int mulDivOps, int& value)
    {
        // 包含 mulDivOps 个乘除运算符的段
        //（禁止 ÷1、÷自身、*1）
        if (mulDivOps < 0) { value = 0; return ""; }
        int current = rand(1, maxNum);
        QString expr = QString::number(current);
        value = current;
        for (int i = 0; i < mulDivOps; ++i) {
            bool canMultiply = timesRtn;
            bool canDivide = divideRtn;
            if (canDivide) {
                int limit = qMin(maxNum, value - 1);
                QVector<int> validDivisors;
                for (int d = 2; d <= limit; ++d)
                    if (value % d == 0)  validDivisors.emplace_back(d);
                if (!validDivisors.isEmpty()) {
                    int divisor = validDivisors[rand(0, validDivisors.size() - 1)];
                    expr += u8"÷" + QString::number(divisor);
                    value /= divisor;
                    continue;
                }
            }
            if (canMultiply) {
                int num;
                if (maxNum > 1)  num = rand(2, maxNum); 
                else   num = 1;
                expr += u8"×" + QString::number(num);
                value *= num;
                continue;
            }
            value = 0;
            return "";
        }
        return expr;
    }
    QString generateQuestion(int seg)
    {
        if (seg < 0) return "";
        if (seg == 0) {
            int num = rand(1, maxNum);
            if (num >= minAns && num <= maxAns) {
                QString s = QString::number(num) + "=";
                ans.emplace_back(QString::number(num));
                questions.emplace_back(s);
                return s;
            }
            else {
                int low = qMax(1, minAns);
                int high = qMin(maxNum, maxAns);
                if (low <= high) {
                    int tryNum = rand(low, high);
                    QString s = QString::number(tryNum) + "=";
                    ans.emplace_back(QString::number(tryNum));
                    questions.emplace_back(s);
                    return s;
                }
                return "";
            }
        }
        int cnt = 0;
        go:if (cnt > 1000000)  return "";
        int maxK = seg + 1;
        QVector<int> validKs;
        QVector<int> weights;
        for (int k = 1; k <= maxK; ++k) {
            int addSubOps = k - 1;
            int mulDivTotal = seg - addSubOps;
            if (mulDivTotal < 0) continue;
            if (addSubOps == 0) {
                validKs.emplace_back(k);
                weights.emplace_back(1); 
            }
            else {
                if (addRtn || minusRtn) {
                    validKs.emplace_back(k);
                    weights.emplace_back(k); 
                }
            }
        }
        if (validKs.isEmpty()) { cnt++; goto go; }
        int totalWeight = 0;
        for (int w : weights) totalWeight += w;
        int r = rand(1, totalWeight);
        int accum = 0;
        int k = validKs[0];
        for (int i = 0; i < validKs.size(); ++i) {
            accum += weights[i];
            if (r <= accum) {
                k = validKs[i];
                break;
            }
        }
        int addSubOps = k - 1;
        int mulDivTotal = seg - addSubOps;
        QVector<int> segMulDiv(k, 0);
        int remaining = mulDivTotal;
        for (int i = 0; i < k - 1; ++i) {
            int give = rand(0, remaining);
            segMulDiv[i] = give;
            remaining -= give;
        }
        segMulDiv[k - 1] = remaining;
        QVector<QString> segments;
        QVector<int> segValues;
        for (int i = 0; i < k; ++i) {
            int val;
            QString segStr = generateSegment(segMulDiv[i], val);
            if (segStr.isEmpty()) { cnt++; goto go; }
            segments.emplace_back(segStr);
            segValues.emplace_back(val);
        }
        QString expr = segments[0];
        int total = segValues[0];
        for (int i = 1; i < k; ++i) {
            QVector<int> ops;
            if (addRtn) ops.emplace_back(1);
            if (minusRtn) ops.emplace_back(-1);
            if (ops.isEmpty()) { cnt++; goto go; }
            int op = ops[rand(0, ops.size() - 1)];
            char opChar = (op == 1) ? '+' : '-';
            if (posRtn && total + op * segValues[i] < 0) { cnt++; goto go; }
            expr += opChar + segments[i];
            total += op * segValues[i];
        }
        if (total < minAns || total > maxAns) {
            cnt++; goto go;
        }
        QString finalExpr = expr + "=";
        ans.emplace_back(QString::number(total));
        questions.emplace_back(finalExpr);
        return finalExpr;
    }

private:
    Ui::ProblemGeneratorClass ui;

    QString m_filepath, m_cachePath;
    QString segNum = "1", totNum = "1";
    QPoint mousePoint;
    QVector<QString> questions, ans;
    QTimer* timer;
	const int edition = 43627436; // 版本号
    int maxNum = 10, colNum = 1, marginNum = 50, minAns = 0, maxAns = 20;
    int mouse_press = 0, posRtn = 1, borderRtn = 0, portraitRtn = 0, ansRtn = 0, total = 1;
	int addRtn = 1, minusRtn = 1, timesRtn = 0, divideRtn = 0;
    double m_rec = 1.0;

    std::mt19937& global_urng() {
        static std::random_device rd;
        static std::mt19937 urng(rd());
        return urng;
    }
};

