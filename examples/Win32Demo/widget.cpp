/*
 * MIT License
 *
 * Copyright (C) 2020 by wangwenx190 (Yuhang Zhao)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "widget.h"
#include "../../winnativeeventfilter.h"
#include "ui_widget.h"
#include <QStyleOption>

namespace {

const char useNativeTitleBar[] = "WNEF_USE_NATIVE_TITLE_BAR";
const char preserveWindowFrame[] = "WNEF_FORCE_PRESERVE_WINDOW_FRAME";

void *getRawHandle(QWidget *widget)
{
    Q_ASSERT(widget);
    return widget->isTopLevel() ? reinterpret_cast<void *>(widget->winId()) : nullptr;
}

void updateWindow(QWidget *widget)
{
    Q_ASSERT(widget);
    if (widget->isTopLevel()) {
        WinNativeEventFilter::updateWindow(getRawHandle(widget), true, true);
    }
}

} // namespace

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);

    connect(ui->minimizeButton, &QPushButton::clicked, this, &Widget::showMinimized);
    connect(ui->maximizeButton, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(ui->closeButton, &QPushButton::clicked, this, &Widget::close);

    connect(ui->moveCenterButton, &QPushButton::clicked, this, [this]() {
        WinNativeEventFilter::moveWindowToDesktopCenter(getRawHandle(this));
    });

    connect(this, &Widget::windowTitleChanged, ui->titleLabel, &QLabel::setText);
    connect(this, &Widget::windowIconChanged, ui->iconButton, &QPushButton::setIcon);

    connect(ui->customizeTitleBarCB, &QCheckBox::stateChanged, this, [this](int state) {
        const bool enable = state == Qt::Checked;
        WinNativeEventFilter::updateQtFrame(windowHandle(),
                                            enable ? WinNativeEventFilter::getSystemMetric(
                                                getRawHandle(this),
                                                WinNativeEventFilter::SystemMetric::TitleBarHeight)
                                                   : 0);
        ui->titleBarWidget->setVisible(enable);
        if (enable) {
            qunsetenv(useNativeTitleBar);
        } else if (state == Qt::Unchecked) {
            qputenv(useNativeTitleBar, "1");
        }
        updateWindow(this);
    });
    connect(ui->windowFrameCB, &QCheckBox::stateChanged, this, [this](int state) {
        if (state == Qt::Checked) {
            qunsetenv(preserveWindowFrame);
        } else if (state == Qt::Unchecked) {
            qputenv(preserveWindowFrame, "1");
        }
        updateWindow(this);
    });
    connect(ui->blurEffectCB, &QCheckBox::stateChanged, this, [this](int state) {
        const bool enable = state == Qt::Checked;
        QPalette palette = {};
        // Choose the gradient color you like.
        palette.setColor(QPalette::Window, QColor(255, 255, 255, 235));
        setPalette(enable ? palette : QPalette());
        WinNativeEventFilter::setBlurEffectEnabled(getRawHandle(this), enable);
        updateWindow(this);
    });
    connect(ui->resizableCB, &QCheckBox::stateChanged, this, [this](int state) {
        const auto data = WinNativeEventFilter::windowData(this);
        if (data) {
            data->fixedSize = state == Qt::Unchecked;
            updateWindow(this);
        }
    });

    QStyleOption option;
    option.initFrom(this);
    setWindowIcon(style()->standardIcon(QStyle::SP_ComputerIcon, &option));
    setWindowTitle(tr("Hello, World!"));

    WinNativeEventFilter::WINDOWDATA data = {};
    data.ignoreObjects << ui->minimizeButton << ui->maximizeButton << ui->closeButton;
    WinNativeEventFilter::addFramelessWindow(this, &data);

    installEventFilter(this);
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::eventFilter(QObject *object, QEvent *event)
{
    Q_ASSERT(object);
    Q_ASSERT(event);
    switch (event->type()) {
    case QEvent::WindowStateChange: {
        if (isMaximized()) {
            ui->maximizeButton->setIcon(QIcon(QLatin1String(":/images/button_restore_black.svg")));
        } else if (!isFullScreen() && !isMinimized()) {
            ui->maximizeButton->setIcon(QIcon(QLatin1String(":/images/button_maximize_black.svg")));
        }
        ui->moveCenterButton->setEnabled(!isMaximized() && !isFullScreen());
    } break;
    case QEvent::WinIdChange:
        WinNativeEventFilter::addFramelessWindow(this);
        break;
    default:
        break;
    }
    return QWidget::eventFilter(object, event);
}