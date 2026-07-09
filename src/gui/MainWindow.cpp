#include "gui/MainWindow.h"

#include "app/Config.h"
#include "app/StartupConfig.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QIcon>
#include <QTabWidget>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <sstream>

namespace shareaudio::gui {
namespace {

QString qstr(const std::string& value)
{
    return QString::fromUtf8(value.data(), static_cast<int>(value.size()));
}

std::string std_str(const QString& value)
{
    QByteArray bytes = value.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

QString mode_label(AudioMode mode)
{
    return qstr(to_string(mode));
}

QString session_mode_label(SessionMode mode)
{
    return qstr(to_string(mode));
}

QString device_label(const AudioDevice& device)
{
    QString label = qstr(device.name);
    if (device.is_default) {
        label += " (default)";
    }
    if (!device.id.empty()) {
        label += " [" + qstr(device.id) + "]";
    }
    return label;
}

AppConfig load_saved_app_config()
{
    auto loaded = load_config_file(default_config_path());
    if (loaded.ok()) {
        return loaded.value();
    }
    return AppConfig {};
}

void select_combo_data(QComboBox* combo, const QVariant& data)
{
    if (!data.isValid()) {
        return;
    }
    const int index = combo->findData(data);
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

void update_button_style(QPushButton* button, const QString& object_name, const QString& text)
{
    button->setText(text);
    if (button->objectName() != object_name) {
        button->setObjectName(object_name);
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , controller_(load_saved_app_config())
{
    build_ui();
    setStyleSheet(R"(
        QMainWindow {
            background-color: #0A0F1D;
        }
        QGroupBox {
            background-color: #151F3C;
            color: #FFFFFF;
            border: 1px solid #25335A;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 16px;
            font-weight: bold;
            font-size: 13px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 4px;
            color: #00A3FF;
        }
        QLabel {
            color: #BAC7DE;
            font-size: 12px;
        }
        QLineEdit {
            background-color: #0A0F1D;
            color: #FFFFFF;
            border: 1px solid #25335A;
            border-radius: 4px;
            padding: 6px;
            font-size: 12px;
        }
        QLineEdit:focus {
            border: 1px solid #00A3FF;
        }
        QComboBox {
            background-color: #0A0F1D;
            color: #FFFFFF;
            border: 1px solid #25335A;
            border-radius: 4px;
            padding: 6px;
            font-size: 12px;
            min-width: 140px;
        }
        QComboBox:focus {
            border: 1px solid #00A3FF;
        }
        QComboBox QAbstractItemView {
            background-color: #0A0F1D;
            color: #FFFFFF;
            selection-background-color: #1DF09A;
            selection-color: #0A0F1D;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1A73E8, stop:1 #0078FF);
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2b82f6, stop:1 #1c85ff);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #155bb5, stop:1 #005fcc);
        }
        QPushButton:disabled {
            background-color: #2D3748;
            color: #718096;
        }
        QPushButton#stopButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #E53935, stop:1 #D32F2F);
            color: #FFFFFF;
        }
        QPushButton#stopButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ef5350, stop:1 #e53935);
        }
        QPushButton#startShareButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1DF09A, stop:1 #00FF88);
            color: #0A0F1D;
        }
        QPushButton#startShareButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #33fcae, stop:1 #24ff9c);
        }
        QPushButton#connectButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0078FF, stop:1 #00C6FF);
            color: #FFFFFF;
        }
        QPushButton#connectButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1c88ff, stop:1 #1cd0ff);
        }
        QTabWidget::pane {
            border: 1px solid #25335A;
            background-color: #151F3C;
            border-radius: 8px;
            top: -1px;
        }
        QTabBar::tab {
            background-color: #0A0F1D;
            color: #BAC7DE;
            border: 1px solid #25335A;
            border-bottom-color: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            padding: 8px 16px;
            margin-right: 2px;
            font-weight: bold;
        }
        QTabBar::tab:selected {
            background-color: #151F3C;
            color: #FFFFFF;
            border-bottom-color: #151F3C;
            font-weight: bold;
        }
        QListWidget {
            background-color: #0A0F1D;
            color: #E2E8F0;
            border: 1px solid #25335A;
            border-radius: 4px;
            padding: 4px;
        }
        QListWidget::item:selected {
            background-color: #0078FF;
            color: #FFFFFF;
        }
        QPlainTextEdit {
            background-color: #0A0F1D;
            color: #A0AEC0;
            font-family: Consolas, monospace;
            font-size: 11px;
            border: 1px solid #25335A;
            border-radius: 4px;
        }
    )");
    refresh_all();
    apply_startup_config();

    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, &QTimer::timeout, this, [this] {
        refresh_status();
    });
    refresh_timer_->start(300);
}

MainWindow::~MainWindow()
{
    save_config_file(default_config_path(), controller_.config_snapshot());
    controller_.stop();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    save_config_file(default_config_path(), controller_.config_snapshot());
    controller_.stop();
    event->accept();
}

void MainWindow::build_ui()
{
    setWindowTitle("ShareAudioLite");
    setWindowIcon(QIcon(":/icon/logo.png"));
    setMinimumSize(800, 300);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    // Global Top Status Indicator (Simple - Line 1)
    auto* status_card = new QGroupBox("Connection Status", central);
    auto* status_layout = new QHBoxLayout(status_card);
    status_layout->setContentsMargins(12, 8, 12, 8);
    
    state_label_ = new QLabel("Ready", status_card);
    state_label_->setStyleSheet("font-weight: bold; font-size: 13px; color: #00FF88;");
    
    ip_info_label_ = new QLabel("-", status_card);
    ip_info_label_->setStyleSheet("font-weight: bold; color: #00C6FF;");
    
    port_label_ = new QLabel(QString::number(Defaults::tcp_port), status_card);
    port_label_->setStyleSheet("font-weight: bold; color: #FFFFFF;");
    
    error_label_ = new QLabel("-", status_card);
    error_label_->setStyleSheet("color: #FF5555;");
    error_label_->setWordWrap(true);
    
    toggle_mode_button_ = new QPushButton("Show Advanced Options", status_card);
    connect(toggle_mode_button_, &QPushButton::clicked, this, [this] {
        advanced_mode_ = !advanced_mode_;
        update_layout_visibility();
    });
    
    status_layout->addWidget(new QLabel("State:", status_card));
    status_layout->addWidget(state_label_);
    status_layout->addWidget(new QLabel("IP/Host:", status_card));
    status_layout->addWidget(ip_info_label_);
    status_layout->addWidget(new QLabel("Port:", status_card));
    status_layout->addWidget(port_label_);
    status_layout->addWidget(new QLabel("Msg:", status_card));
    status_layout->addWidget(error_label_, 1);
    status_layout->addWidget(toggle_mode_button_);
    
    root->addWidget(status_card);

    // --- LINE 2: Server (Transmitter) Card ---
    auto* server_card = new QGroupBox("Server (Transmitter)", central);
    auto* server_layout = new QHBoxLayout(server_card);
    server_layout->setContentsMargins(12, 8, 12, 8);
    server_layout->setSpacing(10);

    // Left part (Always visible): Title and Start/Stop Button
    auto* server_simple_widget = new QWidget(server_card);
    auto* server_simple_layout = new QHBoxLayout(server_simple_widget);
    server_simple_layout->setContentsMargins(0, 0, 0, 0);
    server_simple_layout->setSpacing(10);
    
    auto* server_title_label = new QLabel("Transmit Audio:", server_simple_widget);
    server_title_label->setStyleSheet("font-weight: bold;");
    start_share_button_ = new QPushButton("Start Sharing", server_simple_widget);
    start_share_button_->setObjectName("startShareButton");
    connect(start_share_button_, &QPushButton::clicked, this, [this] {
        const auto status = controller_.status_snapshot();
        if (status.mode == SessionMode::Sharing) {
            stop_session();
        } else {
            start_sharing();
        }
    });
    server_simple_layout->addWidget(server_title_label);
    server_simple_layout->addWidget(start_share_button_);
    server_layout->addWidget(server_simple_widget);

    // Right part (Advanced panel)
    server_advanced_widget_ = new QWidget(server_card);
    auto* server_adv_layout = new QHBoxLayout(server_advanced_widget_);
    server_adv_layout->setContentsMargins(0, 0, 0, 0);
    server_adv_layout->setSpacing(10);

    mode_combo_ = new QComboBox(server_advanced_widget_);
    mode_combo_->addItem("Balanced (Recommended)", QVariant::fromValue(static_cast<int>(AudioMode::Balanced)));
    mode_combo_->addItem("Fast (Low Latency)", QVariant::fromValue(static_cast<int>(AudioMode::Fast)));
    mode_combo_->addItem("Efficient (Low Data)", QVariant::fromValue(static_cast<int>(AudioMode::Efficient)));
    
    capture_combo_ = new QComboBox(server_advanced_widget_);
    
    clients_label_ = new QLabel("0", server_advanced_widget_);
    clients_label_->setStyleSheet("font-weight: bold; color: #FFFFFF;");

    share_form_ = new QFormLayout();
    share_form_->setSpacing(6);
    share_form_->addRow("AudioMode:", mode_combo_);
    share_form_->addRow("Audio Device:", capture_combo_);
    share_form_->addRow("Clients:", clients_label_);

    server_adv_layout->addLayout(share_form_);
    server_layout->addWidget(server_advanced_widget_);
    root->addWidget(server_card);

    // --- LINE 3: Client (Receiver) Card ---
    auto* listen_card = new QGroupBox("Client (Receiver)", central);
    auto* listen_layout = new QHBoxLayout(listen_card);
    listen_layout->setContentsMargins(12, 8, 12, 8);
    listen_layout->setSpacing(10);

    // Left part (Always visible): Input and Connect Button
    auto* client_simple_widget = new QWidget(listen_card);
    auto* client_simple_layout = new QHBoxLayout(client_simple_widget);
    client_simple_layout->setContentsMargins(0, 0, 0, 0);
    client_simple_layout->setSpacing(10);

    auto* client_title_label = new QLabel("Receive Audio:", client_simple_widget);
    client_title_label->setStyleSheet("font-weight: bold;");
    
    host_input_ = new QLineEdit(client_simple_widget);
    host_input_->setPlaceholderText("Transmitter IP (e.g. 192.168.1.50)");
    
    connect_button_ = new QPushButton("Connect Receiver", client_simple_widget);
    connect_button_->setObjectName("connectButton");
    connect(connect_button_, &QPushButton::clicked, this, [this] {
        const auto status = controller_.status_snapshot();
        if (status.mode == SessionMode::Listening || status.mode == SessionMode::Connecting) {
            stop_session();
        } else {
            start_listening();
        }
    });

    client_simple_layout->addWidget(client_title_label);
    client_simple_layout->addWidget(host_input_);
    client_simple_layout->addWidget(connect_button_);
    listen_layout->addWidget(client_simple_widget);

    // Right part (Advanced panel)
    client_advanced_widget_ = new QWidget(listen_card);
    auto* client_adv_layout = new QHBoxLayout(client_advanced_widget_);
    client_adv_layout->setContentsMargins(0, 0, 0, 0);
    client_adv_layout->setSpacing(10);

    playback_combo_ = new QComboBox(client_advanced_widget_);
    
    listen_mode_label_ = new QLabel("-", client_advanced_widget_);
    listen_mode_label_->setStyleSheet("font-weight: bold; color: #FFFFFF;");

    listen_form_ = new QFormLayout();
    listen_form_->setSpacing(6);
    listen_form_->addRow("Audio Speaker:", playback_combo_);
    listen_form_->addRow("Stream Mode:", listen_mode_label_);

    client_adv_layout->addLayout(listen_form_);
    listen_layout->addWidget(client_advanced_widget_);
    root->addWidget(listen_card);

    // --- LINE 4: Tabs Widget ---
    tabs_ = new QTabWidget(central);
    tabs_->setObjectName("mainTabs");

    // Tab 1: Network & History
    auto* dev_tab = new QWidget(tabs_);
    auto* dev_layout = new QHBoxLayout(dev_tab);
    dev_layout->setSpacing(12);
    dev_layout->setContentsMargins(10, 10, 10, 10);

    auto* net_card = new QGroupBox("Network Status", dev_tab);
    auto* net_layout = new QVBoxLayout(net_card);
    local_ips_list_ = new QListWidget(net_card);
    recent_devices_list_ = new QListWidget(net_card);
    connect(recent_devices_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            host_input_->setText(item->text());
        }
    });

    auto* ip_buttons = new QHBoxLayout();
    auto* refresh_ips_button = new QPushButton("Refresh", net_card);
    auto* copy_ip_button = new QPushButton("Copy Selected IP", net_card);
    connect(refresh_ips_button, &QPushButton::clicked, this, [this] {
        refresh_ips();
    });
    connect(copy_ip_button, &QPushButton::clicked, this, [this] {
        copy_selected_ip();
    });
    ip_buttons->addWidget(refresh_ips_button);
    ip_buttons->addWidget(copy_ip_button);

    net_layout->addWidget(new QLabel("Your Local IP Addresses:", net_card));
    net_layout->addWidget(local_ips_list_);
    net_layout->addLayout(ip_buttons);
    net_layout->addWidget(new QLabel("Recent Hosts (Double-click to set):", net_card));
    net_layout->addWidget(recent_devices_list_);
    dev_layout->addWidget(net_card);

    // Tab 2: Hardware Audio Devices
    auto* devices_box = new QGroupBox("Hardware Audio Devices", dev_tab);
    auto* devices_layout = new QVBoxLayout(devices_box);
    capture_devices_list_ = new QListWidget(devices_box);
    playback_devices_list_ = new QListWidget(devices_box);
    auto* refresh_devices_button = new QPushButton("Refresh Devices List", devices_box);
    connect(refresh_devices_button, &QPushButton::clicked, this, [this] {
        refresh_devices();
    });
    devices_layout->addWidget(new QLabel("Available Input/Capture Sources:", devices_box));
    devices_layout->addWidget(capture_devices_list_);
    devices_layout->addWidget(new QLabel("Available Output/Playback Speakers:", devices_box));
    devices_layout->addWidget(playback_devices_list_);
    devices_layout->addWidget(refresh_devices_button);
    dev_layout->addWidget(devices_box);

    tabs_->addTab(dev_tab, "Network & Hardware");

    // Tab 3: Diagnostics & Help
    auto* diag_tab = new QWidget(tabs_);
    auto* diag_layout = new QVBoxLayout(diag_tab);
    diag_layout->setSpacing(10);
    diag_layout->setContentsMargins(10, 10, 10, 10);

    auto* stats_row = new QHBoxLayout();
    bytes_sent_label_ = new QLabel("0", diag_tab);
    packets_label_ = new QLabel("0", diag_tab);
    dropped_label_ = new QLabel("0", diag_tab);
    bytes_received_label_ = new QLabel("0", diag_tab);
    bytes_played_label_ = new QLabel("0", diag_tab);
    buffer_label_ = new QLabel("0", diag_tab);
    underruns_label_ = new QLabel("0", diag_tab);

    auto* stat_form_l = new QFormLayout();
    stat_form_l->addRow("Bytes Sent:", bytes_sent_label_);
    stat_form_l->addRow("Packets Produced:", packets_label_);
    stat_form_l->addRow("Packets Dropped:", dropped_label_);
    stats_row->addLayout(stat_form_l);

    auto* stat_form_r = new QFormLayout();
    stat_form_r->addRow("Bytes Received:", bytes_received_label_);
    stat_form_r->addRow("Bytes Played:", bytes_played_label_);
    stat_form_r->addRow("Jitter Buffer Bytes:", buffer_label_);
    stat_form_r->addRow("Playback Underruns:", underruns_label_);
    stats_row->addLayout(stat_form_r);

    diag_layout->addLayout(stats_row);

    log_view_ = new QPlainTextEdit(diag_tab);
    log_view_->setReadOnly(true);
    diag_layout->addWidget(new QLabel("System Event Log:", diag_tab));
    diag_layout->addWidget(log_view_);

    auto* diag_buttons = new QHBoxLayout();
    auto* copy_diag_button = new QPushButton("Copy Diagnostics to Clipboard", diag_tab);
    auto* help_button = new QPushButton("Help Guide", diag_tab);
    connect(copy_diag_button, &QPushButton::clicked, this, [this] {
        copy_diagnostics();
    });
    connect(help_button, &QPushButton::clicked, this, [this] {
        show_help();
    });
    diag_buttons->addWidget(copy_diag_button);
    diag_buttons->addWidget(help_button);
    diag_layout->addLayout(diag_buttons);

    tabs_->addTab(diag_tab, "Diagnostics & Help");

    root->addWidget(tabs_);
    setCentralWidget(central);

    update_layout_visibility();
}

void MainWindow::update_layout_visibility()
{
    if (server_advanced_widget_) {
        server_advanced_widget_->setVisible(advanced_mode_);
    }
    if (client_advanced_widget_) {
        client_advanced_widget_->setVisible(advanced_mode_);
    }
    if (tabs_) {
        tabs_->setVisible(advanced_mode_);
    }

    if (advanced_mode_) {
        setMinimumSize(1000, 600);
        setMaximumSize(16777215, 16777215);
        resize(1100, 730);
        if (toggle_mode_button_) {
            toggle_mode_button_->setText("Hide Advanced Options");
        }
    } else {
        setMinimumSize(800, 300);
        setMaximumSize(16777215, 16777215);
        resize(830, 310);
        if (toggle_mode_button_) {
            toggle_mode_button_->setText("Show Advanced Options");
        }
    }
}

void MainWindow::refresh_all()
{
    refresh_ips();
    refresh_devices();
    const auto config = controller_.config_snapshot();
    select_combo_data(mode_combo_, QVariant::fromValue(static_cast<int>(config.transmitter.mode)));
    if (!config.receiver.host.empty()) {
        host_input_->setText(qstr(config.receiver.host));
    }
    refresh_recent_devices();
    refresh_status();
}

void MainWindow::refresh_status()
{
    const auto status = controller_.status_snapshot();
    const bool idle = status.mode == SessionMode::Idle;
    const bool sharing = status.mode == SessionMode::Sharing;
    const bool listening = status.mode == SessionMode::Listening || status.mode == SessionMode::Connecting;

    state_label_->setText(session_mode_label(status.mode));
    port_label_->setText(QString::number(status.port));
    
    // Format IP info label depending on the mode initiated
    if (sharing) {
        QString ips_str;
        const auto ips = controller_.list_local_ips();
        if (!ips.empty()) {
            ips_str = qstr(ips.front());
            if (ips.size() > 1) {
                ips_str += QString(" (+%1 more)").arg(ips.size() - 1);
            }
        } else {
            ips_str = "No IP found";
        }
        ip_info_label_->setText(ips_str);
    } else if (listening) {
        ip_info_label_->setText(qstr(status.host));
    } else {
        ip_info_label_->setText("-");
    }

    error_label_->setText(status.last_error.empty() ? "-" : qstr(status.last_error));
    clients_label_->setText(QString::number(status.connected_clients));
    bytes_sent_label_->setText(QString::number(status.bytes_sent));
    packets_label_->setText(QString::number(status.packets_produced));
    dropped_label_->setText(QString::number(status.dropped_packets));
    listen_mode_label_->setText(status.has_detected_mode ? mode_label(status.detected_mode) : "-");
    bytes_received_label_->setText(QString::number(status.bytes_received));
    bytes_played_label_->setText(QString::number(status.bytes_played));
    buffer_label_->setText(QString::number(status.jitter_buffer_depth));
    underruns_label_->setText(QString::number(status.underruns));

    // Server button toggle state and text
    if (sharing) {
        update_button_style(start_share_button_, "stopButton", "Stop Sharing");
    } else {
        update_button_style(start_share_button_, "startShareButton", "Start Sharing");
    }

    // Client button toggle state and text
    if (listening) {
        update_button_style(connect_button_, "stopButton", "Disconnect");
    } else {
        update_button_style(connect_button_, "connectButton", "Connect Receiver");
    }

    start_share_button_->setEnabled(idle || sharing);
    connect_button_->setEnabled(idle || listening);
    mode_combo_->setEnabled(idle);
    capture_combo_->setEnabled(idle);
    playback_combo_->setEnabled(idle);
    host_input_->setEnabled(idle);

    QString logs;
    for (const auto& event : status.log_events) {
        logs += qstr(event) + "\n";
    }
    log_view_->setPlainText(logs);
    refresh_recent_devices();
}

void MainWindow::refresh_ips()
{
    local_ips_list_->clear();
    for (const auto& ip : controller_.list_local_ips()) {
        local_ips_list_->addItem(qstr(ip));
    }
}

void MainWindow::refresh_devices()
{
    capture_combo_->clear();
    playback_combo_->clear();
    capture_devices_list_->clear();
    playback_devices_list_->clear();

    for (const auto& device : controller_.list_capture_devices()) {
        capture_combo_->addItem(device_label(device), qstr(device.id));
        capture_devices_list_->addItem(device_label(device));
    }
    for (const auto& device : controller_.list_playback_devices()) {
        playback_combo_->addItem(device_label(device), qstr(device.id));
        playback_devices_list_->addItem(device_label(device));
    }

    const auto config = controller_.config_snapshot();

    // Find the default capture device
    std::string default_capture_id;
    for (const auto& device : controller_.list_capture_devices()) {
        if (device.is_default) {
            default_capture_id = device.id;
            break;
        }
    }

    // Find the default playback device
    std::string default_playback_id;
    for (const auto& device : controller_.list_playback_devices()) {
        if (device.is_default) {
            default_playback_id = device.id;
            break;
        }
    }

    // Use saved device from config if present, otherwise select the default device
    std::string select_capture = config.transmitter.capture_device_id.empty() ? default_capture_id : config.transmitter.capture_device_id;
    std::string select_playback = config.receiver.playback_device_id.empty() ? default_playback_id : config.receiver.playback_device_id;

    if (select_capture.empty() && !controller_.list_capture_devices().empty()) {
        select_capture = controller_.list_capture_devices().front().id;
    }
    if (select_playback.empty() && !controller_.list_playback_devices().empty()) {
        select_playback = controller_.list_playback_devices().front().id;
    }

    select_combo_data(capture_combo_, qstr(select_capture));
    select_combo_data(playback_combo_, qstr(select_playback));

    // Highlight the defaults/saved selections in the lists as well
    for (int i = 0; i < capture_combo_->count(); ++i) {
        if (capture_combo_->itemData(i).toString() == qstr(select_capture)) {
            capture_devices_list_->setCurrentRow(i);
            break;
        }
    }
    for (int i = 0; i < playback_combo_->count(); ++i) {
        if (playback_combo_->itemData(i).toString() == qstr(select_playback)) {
            playback_devices_list_->setCurrentRow(i);
            break;
        }
    }
}

void MainWindow::refresh_recent_devices()
{
    const auto entries = controller_.recent_devices();
    if (recent_devices_list_->count() == static_cast<int>(entries.size())) {
        bool unchanged = true;
        for (int i = 0; i < recent_devices_list_->count(); ++i) {
            if (recent_devices_list_->item(i)->text() != qstr(entries[static_cast<std::size_t>(i)])) {
                unchanged = false;
                break;
            }
        }
        if (unchanged) {
            return;
        }
    }

    recent_devices_list_->clear();
    for (const auto& entry : entries) {
        recent_devices_list_->addItem(qstr(entry));
    }
}

void MainWindow::start_sharing()
{
    const auto selected = static_cast<AudioMode>(mode_combo_->currentData().toInt());
    const auto device_id = std_str(capture_combo_->currentData().toString());
    auto result = controller_.start_sharing(selected, device_id);
    if (!result.ok()) {
        show_error(qstr(result.error().message));
    } else {
        save_config_file(default_config_path(), controller_.config_snapshot());
    }
    refresh_status();
}

void MainWindow::start_listening()
{
    const QString host = host_input_->text().trimmed();
    if (host.isEmpty()) {
        show_error("Host/IP is required.");
        return;
    }
    const auto device_id = std_str(playback_combo_->currentData().toString());
    auto result = controller_.start_listening(std_str(host), device_id);
    if (!result.ok()) {
        show_error(qstr(result.error().message));
    } else {
        save_config_file(default_config_path(), controller_.config_snapshot());
    }
    refresh_status();
}

void MainWindow::stop_session()
{
    controller_.stop();
    save_config_file(default_config_path(), controller_.config_snapshot());
    refresh_status();
}

void MainWindow::copy_selected_ip()
{
    const auto* item = local_ips_list_->currentItem();
    if (item) {
        QApplication::clipboard()->setText(item->text());
    }
}

void MainWindow::copy_diagnostics()
{
    QApplication::clipboard()->setText(diagnostics_text());
}

void MainWindow::show_help()
{
    QMessageBox::information(
        this,
        "ShareAudioLite Help",
        "Share starts a transmitter on TCP port 8080.\n"
        "Balanced is the default AudioMode. Fast uses smaller PCM packets.\n"
        "Efficient uses Opus when this build is linked with libopus.\n"
        "Listen connects to another machine and autodetects the stream mode from SAL1 or HTTP metadata.\n"
        "Browser/mobile compatibility is exposed through /info, /stream, and the browser player route.");
}

void MainWindow::show_error(const QString& message)
{
    error_label_->setText(message);
    QMessageBox::warning(this, "ShareAudioLite", message);
}

QString MainWindow::diagnostics_text() const
{
    const auto status = controller_.status_snapshot();
    std::ostringstream out;
    out << "ShareAudioLite " << SHAREAUDIO_VERSION << "\n";
    out << "state=" << to_string(status.mode) << "\n";
    out << "port=" << status.port << "\n";
    out << "selected_mode=" << to_string(status.selected_mode) << "\n";
    out << "detected_mode=" << (status.has_detected_mode ? to_string(status.detected_mode) : "-") << "\n";
    out << "host=" << status.host << "\n";
    out << "connected_clients=" << status.connected_clients << "\n";
    out << "bytes_sent=" << status.bytes_sent << "\n";
    out << "bytes_received=" << status.bytes_received << "\n";
    out << "bytes_played=" << status.bytes_played << "\n";
    out << "dropped_packets=" << status.dropped_packets << "\n";
    out << "underruns=" << status.underruns << "\n";
    out << "last_error=" << status.last_error << "\n";
    out << "local_ips=";
    for (const auto& ip : controller_.list_local_ips()) {
        out << ip << " ";
    }
    out << "\nlogs:\n";
    for (const auto& event : status.log_events) {
        out << "- " << event << "\n";
    }
    return qstr(out.str());
}

void MainWindow::apply_startup_config()
{
    auto cfg_result = load_default_startup_config();
    if (!cfg_result.ok()) {
        return; // File doesn't exist or unreadable — silently skip
    }

    const auto& cfg = cfg_result.value();

    // Pre-fill GUI fields regardless of AUTOSTART
    if (cfg.has_audio_mode()) {
        auto parsed = cfg.parsed_audio_mode();
        if (parsed) {
            select_combo_data(mode_combo_, QVariant::fromValue(static_cast<int>(*parsed)));
        }
    }
    if (cfg.has_device_id()) {
        select_combo_data(capture_combo_, qstr(cfg.device_id));
    }
    if (cfg.has_playback_device_id()) {
        select_combo_data(playback_combo_, qstr(cfg.playback_device_id));
    }
    if (cfg.has_server_ip()) {
        host_input_->setText(qstr(cfg.server_ip));
    }

    // Auto-start only if AUTOSTART=true
    if (!cfg.autostart) {
        return;
    }

    // Validate before auto-starting
    if (!cfg.has_mode()) {
        // MODE missing — skip autostart, open normally
        return;
    }

    if (cfg.is_client() && !cfg.has_server_ip()) {
        // Client mode without SERVER_IP — skip autostart, open normally
        return;
    }

    // Defer auto-start until after the event loop starts so the window is fully visible
    if (cfg.is_server()) {
        QTimer::singleShot(200, this, [this] {
            start_sharing();
        });
    } else if (cfg.is_client()) {
        QTimer::singleShot(200, this, [this] {
            start_listening();
        });
    }
}

} // namespace shareaudio::gui
