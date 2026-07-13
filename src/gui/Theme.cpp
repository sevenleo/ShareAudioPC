#include "gui/Theme.h"

namespace shareaudio::gui {

QString dark_theme_stylesheet()
{
    return QStringLiteral(R"(
        QMainWindow { background: #111315; }
        QWidget { background: transparent; color: #F2F5F7; font-family: "Segoe UI Variable Text", "Segoe UI", "Noto Sans"; font-size: 12px; }
        QWidget#centralWorkspace { background: #111315; }
        QGroupBox { background: #191D21; color: #F2F5F7; border: 1px solid #30373E; border-radius: 7px; margin-top: 12px; padding: 16px 14px 12px 14px; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 12px; padding: 0 5px; color: #B9C2CA; }
        QGroupBox#statusPanel { background: #171B1E; border-color: #30373E; }
        QGroupBox#sharingPanel[active="true"] { border-color: #22D68C; }
        QGroupBox#sharingPanel[active="true"]::title { color: #22D68C; }
        QGroupBox#receiverPanel[state="connecting"] { border-color: #F5A524; }
        QGroupBox#receiverPanel[state="connecting"]::title { color: #F5A524; }
        QGroupBox#receiverPanel[state="active"] { border-color: #2F9BFF; }
        QGroupBox#receiverPanel[state="active"]::title { color: #2F9BFF; }
        QGroupBox#statusPanel[stateTone="error"] { border-color: #E5484D; }
        QLabel { background: transparent; color: #AAB3BC; }
        QLabel#stateValue { color: #C4CBD1; font-weight: 700; }
        QLabel#statusCaption { color: #77838D; font-size: 10px; font-weight: 600; }
        QLabel#stateValue[tone="sharing"] { color: #22D68C; }
        QLabel#stateValue[tone="connecting"] { color: #F5A524; }
        QLabel#stateValue[tone="listening"] { color: #2F9BFF; }
        QLabel#stateValue[tone="combined"] { color: #F2F5F7; }
        QLabel#stateValue[tone="error"] { color: #FF7278; }
        QLabel#addressValue { color: #D7DEE4; font-family: Consolas, "Cascadia Mono"; font-weight: 600; }
        QLabel#portValue, QLabel#metricValue { color: #F2F5F7; font-family: Consolas, "Cascadia Mono"; font-weight: 600; }
        QLabel#errorValue { color: #7E8992; }
        QLabel#errorValue[hasError="true"] { background: #2B1B1D; color: #FF9B9F; border: 1px solid #6B3035; border-radius: 4px; padding: 5px 8px; }
        QLabel#sectionLabel { color: #F2F5F7; font-weight: 600; }
        QWidget#footerBar { background: #0D0F10; border-top: 1px solid #2A3035; }
        QLabel#footerVersion { color: #7E8992; font-family: Consolas, "Cascadia Mono"; font-size: 10px; }
        QLineEdit, QComboBox { background: #12161A; color: #F2F5F7; border: 1px solid #3B4249; border-radius: 6px; padding: 7px 9px; min-height: 22px; selection-background-color: #245E91; }
        QLineEdit:hover, QComboBox:hover { border-color: #59636D; }
        QLineEdit:focus, QComboBox:focus { border-color: #2F9BFF; }
        QLineEdit:disabled, QComboBox:disabled { color: #68727B; border-color: #2A3035; background: #15181A; }
        QComboBox QAbstractItemView { background: #202428; color: #F2F5F7; border: 1px solid #3B4249; selection-background-color: #245E91; selection-color: #FFFFFF; }
        QPushButton { background: #252C32; color: #F2F5F7; border: 1px solid #3B4249; border-radius: 6px; padding: 8px 14px; min-height: 24px; font-weight: 600; }
        QPushButton:hover { background: #353D43; border-color: #68727B; }
        QPushButton:pressed { background: #20252A; }
        QPushButton:focus { border-color: #2F9BFF; }
        QPushButton:disabled { background: #1A1E21; color: #68727B; border-color: #2A3035; }
        QPushButton#startShareButton { background: #22D68C; color: #082519; border-color: #22D68C; min-width: 132px; }
        QPushButton#startShareButton:hover { background: #3AE6A0; border-color: #3AE6A0; }
        QPushButton#connectButton { background: #2F9BFF; color: #081B2D; border-color: #2F9BFF; min-width: 142px; }
        QPushButton#connectButton:hover { background: #55AEFF; border-color: #55AEFF; }
        QPushButton#stopButton { background: #E5484D; color: #FFF7F7; border-color: #E5484D; min-width: 142px; }
        QPushButton#stopButton:hover { background: #F2555A; border-color: #F2555A; }
        QPushButton#secondaryButton { background: #252C32; color: #D7DEE4; }
        QCheckBox { background: transparent; color: #C4CBD1; spacing: 8px; }
        QCheckBox:hover { color: #F2F5F7; }
        QCheckBox:disabled { color: #68727B; }
        QCheckBox::indicator { width: 16px; height: 16px; }
        QCheckBox#trayModeCheckbox { color: #D7DEE4; font-size: 13px; font-weight: 600; padding: 5px 8px; }
        QTabWidget::pane { background: #191D21; border: 1px solid #30373E; border-radius: 7px; top: -1px; }
        QTabBar::tab { background: #111315; color: #8D98A1; border: 1px solid #343A40; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; padding: 7px 14px; margin-right: 3px; font-weight: 600; }
        QTabBar::tab:hover { color: #F2F5F7; background: #202428; }
        QTabBar::tab:selected { background: #191D21; color: #F2F5F7; border-color: #59636D; }
        QListWidget, QPlainTextEdit { background: #12161A; color: #D7DEE4; border: 1px solid #343A40; border-radius: 6px; padding: 4px; }
        QListWidget::item { padding: 5px 7px; }
        QListWidget::item:hover { background: #202428; }
        QListWidget::item:selected { background: #245E91; color: #FFFFFF; }
        QPlainTextEdit { font-family: Consolas, "Cascadia Mono"; font-size: 11px; selection-background-color: #245E91; }
        QScrollBar:vertical { background: #111315; width: 10px; margin: 0; }
        QScrollBar::handle:vertical { background: #3B4249; min-height: 24px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: #59636D; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QMenu { background: #202428; color: #F2F5F7; border: 1px solid #3B4249; padding: 4px; }
        QMenu::item { padding: 7px 24px 7px 10px; border-radius: 4px; }
        QMenu::item:selected { background: #245E91; }
        QToolTip { background: #202428; color: #F2F5F7; border: 1px solid #59636D; padding: 4px; }
    )");
}

} // namespace shareaudio::gui
