#include "gui/Theme.h"

namespace shareaudio::gui {

QString app_theme_stylesheet()
{
    return QStringLiteral(R"(
        QMainWindow { background: #F7F8FA; }
        QWidget { background: transparent; color: #111827; font-family: "Segoe UI Variable Text", "Segoe UI", "Noto Sans"; font-size: 13px; }
        QWidget#centralWorkspace { background: #F7F8FA; }
        QWidget#primarySessionArea { background: transparent; }
        QGroupBox { background: #FFFFFF; color: #111827; border: 1px solid #E5E7EB; border-radius: 6px; margin-top: 0; padding: 14px; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 4px; color: #111827; }
        QGroupBox#statusPanel { background: #FFFFFF; border: 1px solid #D8DEE6; padding: 0; min-height: 44px; max-height: 46px; }
        QGroupBox#sharingPanel { border-left: 4px solid #16A34A; }
        QGroupBox#sharingPanel[active="true"] { border-color: #16A34A; }
        QGroupBox#receiverPanel { border-left: 4px solid #2563EB; }
        QGroupBox#receiverPanel[state="connecting"] { border-left-color: #F59E0B; }
        QGroupBox#receiverPanel[state="active"] { border-left-color: #2563EB; }
        QGroupBox#subPanel { padding: 12px; }
        QLabel { background: transparent; color: #111827; }
        QLabel#statusDot { color: #16A34A; font-size: 18px; }
        QLabel#stateValue { color: #111827; font-weight: 700; }
        QLabel#statusCaption { color: #111827; font-size: 13px; font-weight: 500; }
        QLabel#fieldCaption { color: #111827; font-size: 13px; font-weight: 600; }
        QLabel#fieldValue { background: #FFFFFF; color: #111827; border: 1px solid #B8C4D3; border-radius: 5px; padding: 6px 9px; font-size: 13px; }
        QLabel#stateValue[tone="sharing"], QLabel#stateValue[tone="combined"] { color: #16A34A; }
        QLabel#stateValue[tone="connecting"] { color: #F59E0B; }
        QLabel#stateValue[tone="listening"] { color: #2563EB; }
        QLabel#stateValue[tone="error"] { color: #EF4444; }
        QLabel#addressValue, QLabel#portValue, QLabel#metricValue { color: #111827; font-weight: 500; }
        QLabel#healthValue { color: #111827; font-weight: 500; padding-left: 12px; }
        QLabel#errorValue { color: #6B7280; }
        QLabel#errorValue[hasError="true"] { background: #FEF2F2; color: #B91C1C; border: 1px solid #FECACA; border-radius: 6px; padding: 8px 10px; }
        QLabel#sectionLabel { color: #111827; font-weight: 700; padding-top: 8px; }
        QLabel#panelTitle { font-size: 16px; font-weight: 800; }
        QLabel#panelTitle[tone="sharing"] { color: #16A34A; }
        QLabel#panelTitle[tone="receiver"] { color: #2563EB; }
        QLabel#clientsBadge { color: #16A34A; font-weight: 700; padding: 5px 8px; }
        QLabel#modeBadge { color: #2563EB; background: #EFF6FF; border: 1px solid #BFDBFE; border-radius: 6px; padding: 7px 12px; font-weight: 600; }
        QLabel#modeBadge[active="false"] { color: #6B7280; background: transparent; border-color: transparent; }
        QLabel#fieldValue[tone="active"] { color: #16A34A; font-weight: 600; }
        QLabel#fieldValue[tone="connecting"] { color: #F59E0B; font-weight: 600; }
        QLabel#linkValue { color: #2563EB; font-weight: 500; }
        QWidget#footerBar { background: #FFFFFF; border-top: 1px solid #E5E7EB; }
        QLabel#footerVersion { color: #6B7280; font-size: 11px; }
        QLineEdit, QComboBox { background: #FFFFFF; color: #111827; border: 1px solid #B8C4D3; border-radius: 6px; padding: 8px 12px; min-height: 24px; selection-background-color: #DBEAFE; selection-color: #111827; }
        QLineEdit:hover, QComboBox:hover { border-color: #7D8EA3; }
        QLineEdit:focus, QComboBox:focus { border-color: #2563EB; }
        QLineEdit:disabled, QComboBox:disabled { color: #9CA3AF; border-color: #DCE3EA; background: #F9FAFB; }
        QComboBox QAbstractItemView { background: #FFFFFF; color: #111827; border: 1px solid #B8C4D3; selection-background-color: #DBEAFE; selection-color: #111827; }
        QPushButton { background: #FFFFFF; color: #111827; border: 1px solid #B8C4D3; border-radius: 6px; padding: 9px 16px; min-height: 24px; font-weight: 700; }
        QPushButton:hover { background: #F8FAFC; border-color: #7D8EA3; }
        QPushButton:pressed { background: #EEF2F7; }
        QPushButton:focus { border-color: #2563EB; }
        QPushButton:disabled { background: #F3F4F6; color: #9CA3AF; border-color: #DCE3EA; }
        QPushButton#startShareButton { background: #FFFFFF; color: #0F7A35; border-color: #86EFAC; min-width: 240px; }
        QPushButton#startShareButton:hover { background: #F0FDF4; border-color: #16A34A; }
        QPushButton#connectButton { background: #FFFFFF; color: #1D4ED8; border-color: #93C5FD; min-width: 240px; }
        QPushButton#connectButton:hover { background: #EFF6FF; border-color: #2563EB; }
        QPushButton#stopButton { background: #FFFFFF; color: #B91C1C; border-color: #FCA5A5; min-width: 240px; }
        QPushButton#stopButton:hover { background: #FEF2F2; border-color: #EF4444; }
        QPushButton#secondaryButton { background: #FFFFFF; color: #111827; min-width: 120px; }
        QPushButton#iconButton { min-width: 34px; max-width: 34px; min-height: 34px; max-height: 34px; padding: 0; }
        QPushButton#quickActionButton { min-width: 106px; min-height: 82px; padding: 10px; font-weight: 600; }
        QCheckBox { background: transparent; color: #111827; spacing: 8px; }
        QCheckBox:hover { color: #000000; }
        QCheckBox:disabled { color: #9CA3AF; }
        QCheckBox::indicator { width: 16px; height: 16px; }
        QCheckBox#trayModeCheckbox { color: #111827; font-size: 14px; padding: 6px 8px; }
        QTabWidget::pane { background: #FFFFFF; border: 1px solid #E5E7EB; border-radius: 6px; top: -1px; }
        QTabBar::tab { background: #F8FAFC; color: #111827; border: 1px solid #E5E7EB; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; padding: 9px 28px; margin-right: 2px; font-weight: 600; }
        QTabBar::tab:hover { background: #FFFFFF; color: #2563EB; }
        QTabBar::tab:selected { background: #FFFFFF; color: #2563EB; border-color: #BFDBFE; }
        QListWidget, QPlainTextEdit { background: #FFFFFF; color: #111827; border: 1px solid #B8C4D3; border-radius: 6px; padding: 4px; }
        QListWidget:hover, QPlainTextEdit:hover { border-color: #7D8EA3; }
        QListWidget:focus, QPlainTextEdit:focus { border-color: #2563EB; }
        QListWidget:disabled, QPlainTextEdit:disabled { color: #9CA3AF; border-color: #DCE3EA; background: #F9FAFB; }
        QListWidget::item { padding: 5px 7px; }
        QListWidget::item:hover { background: #F8FAFC; }
        QListWidget::item:selected { background: #DBEAFE; color: #111827; }
        QPlainTextEdit { font-family: Consolas, "Cascadia Mono"; font-size: 11px; selection-background-color: #DBEAFE; }
        QSlider::groove:horizontal { height: 6px; background: #D1D5DB; border-radius: 3px; }
        QSlider::sub-page:horizontal { background: #2563EB; border-radius: 3px; }
        QSlider::handle:horizontal { background: #FFFFFF; border: 1px solid #D1D5DB; width: 18px; height: 18px; margin: -7px 0; border-radius: 9px; }
        QScrollBar:vertical { background: #F7F8FA; width: 10px; margin: 0; }
        QScrollBar::handle:vertical { background: #CBD5E1; min-height: 24px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: #94A3B8; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QMenu { background: #FFFFFF; color: #111827; border: 1px solid #D1D5DB; padding: 4px; }
        QMenu::item { padding: 7px 24px 7px 10px; border-radius: 4px; }
        QMenu::item:selected { background: #DBEAFE; }
        QToolTip { background: #111827; color: #FFFFFF; border: 1px solid #111827; padding: 4px; }
    )");
}

} // namespace shareaudio::gui
