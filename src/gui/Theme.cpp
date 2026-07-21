#include "gui/Theme.h"

namespace shareaudio::gui {

QString app_theme_stylesheet(bool dark_mode)
{
    if (dark_mode) {
        return QStringLiteral(R"(
            QMainWindow, QMessageBox { background-color: #0A0F1D; }
            QGroupBox { background-color: #151F3C; color: #FFFFFF; border: 1px solid #25335A; border-radius: 8px; margin-top: 12px; padding-top: 16px; font-weight: bold; font-size: 13px; }
            QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 12px; padding: 0 4px; color: #00A3FF; }
            QLabel, QCheckBox { color: #BAC7DE; font-size: 12px; }
            QLabel#stateValue { color: #00FF88; font-weight: bold; font-size: 13px; }
            QLabel#addressValue { color: #00C6FF; font-weight: bold; }
            QLabel#portValue, QLabel#clientsValue, QLabel#modeValue { color: #FFFFFF; font-weight: bold; }
            QLabel#errorValue { color: #FF5555; }
            QCheckBox:disabled { color: #718096; }
            QCheckBox#followSystemVolumeCheckbox, QCheckBox#muteLocalAudioCheckbox { color: #FFFFFF; padding: 4px 2px; }
            QWidget#footerBar { background-color: #0A0F1D; border-top: 1px solid #25335A; }
            QCheckBox#trayModeCheckbox { color: #FFFFFF; font-size: 14px; font-weight: bold; spacing: 10px; padding: 7px 10px; }
            QCheckBox#trayModeCheckbox:hover { color: #1DF09A; }
            QCheckBox#trayModeCheckbox:disabled { color: #718096; }
            QCheckBox#trayModeCheckbox::indicator { width: 18px; height: 18px; }
            QLineEdit, QComboBox { background-color: #0A0F1D; color: #FFFFFF; border: 1px solid #25335A; border-radius: 4px; padding: 6px; font-size: 12px; }
            QLineEdit:focus, QComboBox:focus { border: 1px solid #00A3FF; }
            QComboBox { min-width: 140px; }
            QComboBox QAbstractItemView { background-color: #0A0F1D; color: #FFFFFF; selection-background-color: #1DF09A; selection-color: #0A0F1D; }
            QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1A73E8, stop:1 #0078FF); color: #FFFFFF; border: none; border-radius: 6px; padding: 8px 16px; font-weight: bold; font-size: 13px; }
            QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2B82F6, stop:1 #1C85FF); }
            QPushButton:pressed { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #155BB5, stop:1 #005FCC); }
            QPushButton:disabled { background-color: #2D3748; color: #718096; }
            QPushButton#stopButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #E53935, stop:1 #D32F2F); color: #FFFFFF; }
            QPushButton#stopButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #EF5350, stop:1 #E53935); }
            QPushButton#startShareButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1DF09A, stop:1 #00FF88); color: #0A0F1D; }
            QPushButton#startShareButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #33FCAE, stop:1 #24FF9C); }
            QTabWidget::pane { border: 1px solid #25335A; background-color: #151F3C; border-radius: 8px; top: -1px; }
            QTabBar::tab { background-color: #0A0F1D; color: #BAC7DE; border: 1px solid #25335A; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; padding: 8px 16px; margin-right: 2px; font-weight: bold; }
            QTabBar::tab:selected { background-color: #151F3C; color: #FFFFFF; border-bottom-color: #151F3C; }
            QListWidget, QPlainTextEdit { background-color: #0A0F1D; color: #E2E8F0; border: 1px solid #25335A; border-radius: 4px; padding: 4px; }
            QListWidget::item:selected { background-color: #0078FF; color: #FFFFFF; }
            QPlainTextEdit { color: #A0AEC0; font-family: Consolas, monospace; font-size: 11px; }
            QSlider::groove:horizontal { height: 5px; background: #25335A; border-radius: 2px; }
            QSlider::sub-page:horizontal { background: #00C6FF; border-radius: 2px; }
            QSlider::handle:horizontal { background: #FFFFFF; width: 14px; margin: -5px 0; border-radius: 7px; }
        )");
    }

    return QStringLiteral(R"(
        QMainWindow, QMessageBox { background-color: #F3F6FB; }
        QGroupBox { background-color: #FFFFFF; color: #172033; border: 1px solid #C9D4E5; border-radius: 8px; margin-top: 12px; padding-top: 16px; font-weight: bold; font-size: 13px; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 12px; padding: 0 4px; color: #006FC9; }
        QLabel, QCheckBox { color: #43516A; font-size: 12px; }
        QLabel#stateValue { color: #008F55; font-weight: bold; font-size: 13px; }
        QLabel#addressValue { color: #006FC9; font-weight: bold; }
        QLabel#portValue, QLabel#clientsValue, QLabel#modeValue { color: #172033; font-weight: bold; }
        QLabel#errorValue { color: #C62828; }
        QCheckBox:disabled { color: #98A4B7; }
        QCheckBox#followSystemVolumeCheckbox, QCheckBox#muteLocalAudioCheckbox { color: #172033; padding: 4px 2px; }
        QWidget#footerBar { background-color: #F3F6FB; border-top: 1px solid #C9D4E5; }
        QCheckBox#trayModeCheckbox { color: #172033; font-size: 14px; font-weight: bold; spacing: 10px; padding: 7px 10px; }
        QCheckBox#trayModeCheckbox:hover { color: #008F55; }
        QCheckBox#trayModeCheckbox:disabled { color: #98A4B7; }
        QCheckBox#trayModeCheckbox::indicator { width: 18px; height: 18px; }
        QLineEdit, QComboBox { background-color: #FFFFFF; color: #172033; border: 1px solid #AEBBD0; border-radius: 4px; padding: 6px; font-size: 12px; }
        QLineEdit:focus, QComboBox:focus { border: 1px solid #0078D4; }
        QComboBox { min-width: 140px; }
        QComboBox QAbstractItemView { background-color: #FFFFFF; color: #172033; selection-background-color: #BFE7FF; selection-color: #172033; }
        QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1A73E8, stop:1 #0078D4); color: #FFFFFF; border: none; border-radius: 6px; padding: 8px 16px; font-weight: bold; font-size: 13px; }
        QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2B82F6, stop:1 #1687DB); }
        QPushButton:pressed { background: #005A9E; }
        QPushButton:disabled { background-color: #D7DEEA; color: #8995A8; }
        QPushButton#stopButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #E53935, stop:1 #C62828); }
        QPushButton#startShareButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #20C77A, stop:1 #00A862); color: #FFFFFF; }
        QTabWidget::pane { border: 1px solid #C9D4E5; background-color: #FFFFFF; border-radius: 8px; top: -1px; }
        QTabBar::tab { background-color: #EAF0F8; color: #43516A; border: 1px solid #C9D4E5; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; padding: 8px 16px; margin-right: 2px; font-weight: bold; }
        QTabBar::tab:selected { background-color: #FFFFFF; color: #172033; border-bottom-color: #FFFFFF; }
        QListWidget, QPlainTextEdit { background-color: #FFFFFF; color: #243149; border: 1px solid #AEBBD0; border-radius: 4px; padding: 4px; }
        QListWidget::item:selected { background-color: #0078D4; color: #FFFFFF; }
        QPlainTextEdit { color: #52617A; font-family: Consolas, monospace; font-size: 11px; }
        QSlider::groove:horizontal { height: 5px; background: #C9D4E5; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #0078D4; border-radius: 2px; }
        QSlider::handle:horizontal { background: #FFFFFF; border: 1px solid #7F91AA; width: 14px; margin: -5px 0; border-radius: 7px; }
    )");
}

} // namespace shareaudio::gui
