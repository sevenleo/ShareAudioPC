#pragma once

#include "app/SessionController.h"

#include <QMainWindow>

class QCloseEvent;
class QAction;
class QCheckBox;
class QComboBox;
class QEvent;
class QFormLayout;
class QGridLayout;
class QGroupBox;
class QHideEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QPlainTextEdit;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QShowEvent;
class QSlider;
class QSystemTrayIcon;
class QTabWidget;
class QTimer;

namespace shareaudio::gui {

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    [[nodiscard]] bool should_start_hidden() const;

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void build_ui();
    void setup_tray();
    void update_layout_visibility();
    void set_tray_mode(bool enabled);
    void show_from_tray();
    void hide_to_tray();
    void toggle_window_visibility();
    void exit_from_tray();
    void update_tray_actions();
    void refresh_all();
    void refresh_status();
    void refresh_ips();
    void refresh_devices();
    void refresh_recent_devices();
    void start_sharing();
    void start_listening();
    void stop_sharing();
    void stop_listening();
    void stop_session();
    void copy_selected_ip();
    void copy_diagnostics();
    void show_help();
    void show_error(const QString& message);
    void apply_startup_config();
    void apply_responsive_layout();
    QString diagnostics_text() const;

    SessionController controller_;
    QTimer* refresh_timer_ {};

    QLabel* state_label_ {};
    QLabel* port_label_ {};
    QLabel* error_label_ {};
    QLabel* ip_info_label_ {};
    QLabel* receiver_summary_label_ {};
    QLabel* clients_label_ {};
    QLabel* simple_clients_label_ {};
    QLabel* share_mode_summary_label_ {};
    QLabel* capture_summary_label_ {};
    QLabel* playback_summary_label_ {};
    QLabel* bytes_sent_label_ {};
    QLabel* packets_label_ {};
    QLabel* dropped_label_ {};
    QLabel* listen_mode_label_ {};
    QLabel* bytes_received_label_ {};
    QLabel* bytes_played_label_ {};
    QLabel* buffer_label_ {};
    QLabel* underruns_label_ {};
    QLabel* receiver_status_label_ {};
    QLabel* volume_gain_label_ {};

    QComboBox* mode_combo_ {};
    QComboBox* capture_combo_ {};
    QComboBox* playback_combo_ {};
    QLineEdit* host_input_ {};
    QListWidget* local_ips_list_ {};
    QListWidget* capture_devices_list_ {};
    QListWidget* playback_devices_list_ {};
    QListWidget* recent_devices_list_ {};
    QPlainTextEdit* log_view_ {};
    QGroupBox* status_panel_ {};
    QGroupBox* sharing_panel_ {};
    QGroupBox* receiver_panel_ {};
    QGroupBox* network_panel_ {};
    QGroupBox* devices_panel_ {};
    QGroupBox* quick_actions_panel_ {};
    QCheckBox* tray_mode_checkbox_ {};
    QCheckBox* follow_system_volume_checkbox_ {};

    QPushButton* start_share_button_ {};
    QPushButton* connect_button_ {};
    QPushButton* toggle_mode_button_ {};
    QPushButton* help_footer_button_ {};
    QSlider* volume_slider_ {};
    QScrollArea* content_scroll_ {};
    QWidget* content_widget_ {};
    QWidget* primary_area_ {};
    QGridLayout* primary_layout_ {};
    QGridLayout* network_hardware_layout_ {};
    QTabWidget* tabs_ {};
    QFormLayout* share_form_ {};
    QFormLayout* listen_form_ {};
    QSystemTrayIcon* tray_icon_ {};
    QMenu* tray_menu_ {};
    QAction* toggle_window_action_ {};
    QAction* tray_mode_action_ {};
    QAction* exit_action_ {};
    QWidget* server_advanced_widget_ {};
    QWidget* client_advanced_widget_ {};
    bool advanced_mode_ { false };
    bool tray_available_ { false };
    bool tray_mode_ { false };
    bool start_hidden_ { false };
    bool force_exit_ { false };
};

} // namespace shareaudio::gui
