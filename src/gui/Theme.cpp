#include "gui/Theme.h"

namespace shareaudio::gui {

namespace {
QString dark_theme_overrides()
{
    return QStringLiteral(R"(
        QMainWindow, QWidget#centralWorkspace { background: #151A21; }
        QWidget { color: #E7EDF5; }
        QGroupBox { background: #202731; color: #E7EDF5; border-color: #3B4654; }
        QGroupBox::title, QLabel { color: #E7EDF5; }
        QGroupBox#statusPanel { background: #202731; border-color: #465568; }
        QGroupBox#sharingPanel { border-left-color: #4ADE80; }
        QGroupBox#sharingPanel[active="true"] { border-color: #4ADE80; }
        QGroupBox#receiverPanel { border-left-color: #60A5FA; }
        QGroupBox#receiverPanel[state="connecting"] { border-left-color: #FBBF24; }
        QGroupBox#receiverPanel[state="active"] { border-left-color: #60A5FA; }
        QLabel#statusDot { color: #4ADE80; }
        QLabel#stateValue, QLabel#statusCaption, QLabel#fieldCaption,
        QLabel#addressValue, QLabel#portValue, QLabel#metricValue,
        QLabel#healthValue { color: #E7EDF5; }
        QLabel#fieldValue { background: #1B222B; color: #E7EDF5; border-color: #526477; }
        QLabel#stateValue[tone="sharing"], QLabel#stateValue[tone="combined"],
        QLabel#fieldValue[tone="active"], QLabel#clientsBadge { color: #4ADE80; }
        QLabel#stateValue[tone="connecting"], QLabel#fieldValue[tone="connecting"] { color: #FBBF24; }
        QLabel#stateValue[tone="listening"], QLabel#linkValue { color: #7DB7FF; }
        QLabel#stateValue[tone="error"] { color: #FCA5A5; }
        QLabel#errorValue { color: #AAB6C6; }
        QLabel#errorValue[hasError="true"] { background: #3A242A; color: #FCA5A5; border-color: #86424D; }
        QLabel#sectionLabel { color: #E7EDF5; }
        QLabel#panelTitle[tone="sharing"] { color: #4ADE80; }
        QLabel#panelTitle[tone="receiver"] { color: #60A5FA; }
        QLabel#modeBadge { color: #93C5FD; background: #1A2D47; border-color: #3A6EA5; }
        QLabel#modeBadge[active="false"] { color: #AAB6C6; background: transparent; border-color: transparent; }
        QWidget#footerBar { background: #202731; border-top-color: #3B4654; }
        QLabel#footerVersion { color: #AAB6C6; }
        QLineEdit, QComboBox { background: #1B222B; color: #E7EDF5; border-color: #526477; selection-background-color: #1E3A5F; selection-color: #F8FAFC; }
        QLineEdit:hover, QComboBox:hover { border-color: #7E91A8; }
        QLineEdit:focus, QComboBox:focus { border-color: #60A5FA; }
        QLineEdit:disabled, QComboBox:disabled { color: #7B8797; border-color: #364152; background: #202731; }
        QComboBox QAbstractItemView { background: #202731; color: #E7EDF5; border-color: #526477; selection-background-color: #1E3A5F; selection-color: #F8FAFC; }
        QPushButton { background: #202731; color: #E7EDF5; border-color: #526477; }
        QPushButton:hover { background: #293442; border-color: #7E91A8; }
        QPushButton:pressed { background: #344152; }
        QPushButton:focus { border-color: #60A5FA; }
        QPushButton:disabled { background: #202731; color: #7B8797; border-color: #364152; }
        QPushButton#startShareButton { background: #1D3026; color: #86EFAC; border-color: #3F8F5B; }
        QPushButton#startShareButton:hover { background: #264A34; border-color: #4ADE80; }
        QPushButton#connectButton { background: #1A2D47; color: #93C5FD; border-color: #3A6EA5; }
        QPushButton#connectButton:hover { background: #223E63; border-color: #60A5FA; }
        QPushButton#stopButton { background: #3A242A; color: #FCA5A5; border-color: #A64C5A; }
        QPushButton#stopButton:hover { background: #542B35; border-color: #FB7185; }
        QCheckBox { color: #E7EDF5; }
        QCheckBox:hover { color: #FFFFFF; }
        QCheckBox:disabled { color: #7B8797; }
        QCheckBox::indicator { background: #1B222B; border: 1px solid #526477; border-radius: 3px; }
        QCheckBox::indicator:checked { background: #2563EB; border-color: #60A5FA; }
        QTabWidget::pane { background: #202731; border-color: #3B4654; }
        QTabBar::tab { background: #1B222B; color: #C7D2E0; border-color: #3B4654; }
        QTabBar::tab:hover { background: #293442; color: #93C5FD; }
        QTabBar::tab:selected { background: #202731; color: #93C5FD; border-color: #3A6EA5; }
        QListWidget, QPlainTextEdit { background: #1B222B; color: #E7EDF5; border-color: #526477; }
        QListWidget:hover, QPlainTextEdit:hover { border-color: #7E91A8; }
        QListWidget:focus, QPlainTextEdit:focus { border-color: #60A5FA; }
        QListWidget:disabled, QPlainTextEdit:disabled { color: #7B8797; border-color: #364152; background: #202731; }
        QListWidget::item:hover { background: #293442; }
        QListWidget::item:selected { background: #1E3A5F; color: #F8FAFC; }
        QSlider::groove:horizontal { background: #526477; }
        QSlider::sub-page:horizontal { background: #60A5FA; }
        QSlider::handle:horizontal { background: #E7EDF5; border-color: #7E91A8; }
        QScrollBar:vertical { background: #151A21; }
        QScrollBar::handle:vertical { background: #526477; }
        QScrollBar::handle:vertical:hover { background: #7E91A8; }
        QMenu { background: #202731; color: #E7EDF5; border-color: #526477; }
        QMenu::item:selected { background: #1E3A5F; }
        QToolTip { background: #0E1319; color: #E7EDF5; border-color: #526477; }
    )");
}
} // namespace

QString app_theme_stylesheet(bool dark_mode)
{
    const auto stylesheet = QStringLiteral(R"(
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
    return dark_mode ? stylesheet + dark_theme_overrides() : stylesheet;
}

} // namespace shareaudio::gui
