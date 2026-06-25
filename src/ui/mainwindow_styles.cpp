#include "ui/mainwindow_styles.hpp"


QString UiStyle::videoStart()
{
    return
        "QPushButton {"
        "background:#1e2126;"
        "color:#38bdf8;"
        "border:1px solid #38bdf8;"
        "border-radius:4px;"
        "padding:5px 14px;"
        "font-weight:bold;"
        "}"
        "QPushButton:hover {"
        "background:#38bdf8;"
        "color:#111417;"
        "}"
        "QPushButton:pressed {"
        "background:#0ea5e9;"
        "}";
}

QString UiStyle::videoStop()
{
    return
        "QPushButton {"
        "background:#38bdf8;"
        "color:#111417;"
        "border:1px solid #38bdf8;"
        "border-radius:4px;"
        "padding:5px 14px;"
        "font-weight:bold;"
        "}"
        "QPushButton:hover {"
        "background:#7dd3fc;"
        "}"
        "QPushButton:pressed {"
        "background:#0ea5e9;"
        "}";
}

QString UiStyle::audioStart()
{
    return
        "QPushButton{"
        "background:#1e2126;"
        "color:#22c55e;"
        "border:1px solid #22c55e;"
        "border-radius:4px;"
        "font-weight:bold;"
        "}";
}


QString UiStyle::audioStop()
{
    return
        "QPushButton{"
        "background:#f59e0b;"
        "color:#111417;"
        "border:1px solid #f59e0b;"
        "border-radius:4px;"
        "font-weight:bold;"
        "}";
}


QString UiStyle::mute()
{
    return
        "QPushButton{"
        "background:#64748b;"
        "color:white;"
        "border:1px solid #64748b;"
        "border-radius:4px;"
        "}";
}

QString UiStyle::unmute()
{
    return
        "QPushButton{"
        "background:#1e2126;"
        "color:#cbd5e1;"
        "border:1px solid #2a2d32;"
        "border-radius:4px;"
        "}";
}


