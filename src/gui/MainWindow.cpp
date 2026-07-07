#include "gui/MainWindow.h"

#include "app/Config.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
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
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <sstream>

namespace shareaudio::gui {
namespace {

QString qstr(const std::string& value)
{
    return QString::fromStdString(value);
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

AppConfig load_startup_config()
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

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , controller_(load_startup_config())
{
    build_ui();
    setStyleSheet(R"(
        QMainWindow {
            background-color: #0B101D;
        }
        QGroupBox {
            background-color: #1C253E;
            color: #FFFFFF;
            border: 1px solid #2A3656;
            border-radius: 8px;
            margin-top: 12px;
            font-weight: bold;
            font-size: 13px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 2px 6px;
            color: #00A3FF;
        }
        QLabel {
            color: #E2E8F0;
            font-size: 12px;
        }
        QLineEdit {
            background-color: #0B101D;
            color: #FFFFFF;
            border: 1px solid #2A3656;
            border-radius: 4px;
            padding: 6px;
            font-size: 12px;
        }
        QLineEdit:focus {
            border: 1px solid #00A3FF;
        }
        QComboBox {
            background-color: #0B101D;
            color: #FFFFFF;
            border: 1px solid #2A3656;
            border-radius: 4px;
            padding: 6px;
            min-width: 120px;
        }
        QComboBox:focus {
            border: 1px solid #00A3FF;
        }
        QComboBox QAbstractItemView {
            background-color: #0B101D;
            color: #FFFFFF;
            selection-background-color: #1DF09A;
            selection-color: #0B101D;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1DF09A, stop:1 #00A3FF);
            color: #0B101D;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #22ffa4, stop:1 #1ab0ff);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #19cf85, stop:1 #008ecc);
        }
        QPushButton:disabled {
            background-color: #2D3748;
            color: #718096;
        }
        QListWidget {
            background-color: #0B101D;
            color: #E2E8F0;
            border: 1px solid #2A3656;
            border-radius: 4px;
            padding: 4px;
        }
        QListWidget::item:selected {
            background-color: #1DF09A;
            color: #0B101D;
        }
        QPlainTextEdit {
            background-color: #0B101D;
            color: #A0AEC0;
            font-family: Consolas, monospace;
            font-size: 11px;
            border: 1px solid #2A3656;
            border-radius: 4px;
        }
    )");
    refresh_all();

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
    setMinimumSize(980, 680);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto* top = new QGroupBox("Status", central);
    auto* top_layout = new QGridLayout(top);
    state_label_ = new QLabel("idle", top);
    port_label_ = new QLabel(QString::number(Defaults::tcp_port), top);
    error_label_ = new QLabel("-", top);
    error_label_->setWordWrap(true);
    stop_button_ = new QPushButton("Stop", top);
    connect(stop_button_, &QPushButton::clicked, this, [this] {
        stop_session();
    });
    top_layout->addWidget(new QLabel("State:", top), 0, 0);
    top_layout->addWidget(state_label_, 0, 1);
    top_layout->addWidget(new QLabel("TCP Port:", top), 0, 2);
    top_layout->addWidget(port_label_, 0, 3);
    top_layout->addWidget(stop_button_, 0, 4);
    top_layout->addWidget(new QLabel("Last error:", top), 1, 0);
    top_layout->addWidget(error_label_, 1, 1, 1, 4);
    root->addWidget(top);

    auto* columns = new QHBoxLayout();
    columns->setSpacing(10);

    auto* share_box = new QGroupBox("Share", central);
    auto* share_layout = new QVBoxLayout(share_box);
    mode_combo_ = new QComboBox(share_box);
    mode_combo_->addItem("Balanced", QVariant::fromValue(static_cast<int>(AudioMode::Balanced)));
    mode_combo_->addItem("Ultrafast", QVariant::fromValue(static_cast<int>(AudioMode::Ultrafast)));
    mode_combo_->addItem("Quality (Coming Soon)", QVariant::fromValue(static_cast<int>(AudioMode::Quality)));
    mode_combo_->setItemData(2, false, Qt::UserRole - 1);
    capture_combo_ = new QComboBox(share_box);
    start_share_button_ = new QPushButton("Start Sharing", share_box);
    connect(start_share_button_, &QPushButton::clicked, this, [this] {
        start_sharing();
    });
    clients_label_ = new QLabel("0", share_box);
    bytes_sent_label_ = new QLabel("0", share_box);
    packets_label_ = new QLabel("0", share_box);
    dropped_label_ = new QLabel("0", share_box);
    auto* share_form = new QFormLayout();
    share_form->addRow("Mode", mode_combo_);
    share_form->addRow("Capture device", capture_combo_);
    share_form->addRow("Connected clients", clients_label_);
    share_form->addRow("Bytes sent", bytes_sent_label_);
    share_form->addRow("Packets", packets_label_);
    share_form->addRow("Dropped", dropped_label_);
    share_layout->addLayout(share_form);
    share_layout->addWidget(start_share_button_);
    columns->addWidget(share_box);

    auto* listen_box = new QGroupBox("Listen", central);
    auto* listen_layout = new QVBoxLayout(listen_box);
    host_input_ = new QLineEdit(listen_box);
    host_input_->setPlaceholderText("Transmitter IP, for example 192.168.1.50");
    playback_combo_ = new QComboBox(listen_box);
    connect_button_ = new QPushButton("Connect", listen_box);
    connect(connect_button_, &QPushButton::clicked, this, [this] {
        start_listening();
    });
    listen_mode_label_ = new QLabel("-", listen_box);
    bytes_received_label_ = new QLabel("0", listen_box);
    bytes_played_label_ = new QLabel("0", listen_box);
    buffer_label_ = new QLabel("0", listen_box);
    underruns_label_ = new QLabel("0", listen_box);
    recent_devices_list_ = new QListWidget(listen_box);
    connect(recent_devices_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            host_input_->setText(item->text());
        }
    });
    auto* listen_form = new QFormLayout();
    listen_form->addRow("Host/IP", host_input_);
    listen_form->addRow("Playback device", playback_combo_);
    listen_form->addRow("Detected mode", listen_mode_label_);
    listen_form->addRow("Bytes received", bytes_received_label_);
    listen_form->addRow("Bytes played", bytes_played_label_);
    listen_form->addRow("Buffer bytes", buffer_label_);
    listen_form->addRow("Underruns", underruns_label_);
    listen_layout->addLayout(listen_form);
    listen_layout->addWidget(connect_button_);
    listen_layout->addWidget(new QLabel("Recent devices", listen_box));
    listen_layout->addWidget(recent_devices_list_);
    columns->addWidget(listen_box);

    root->addLayout(columns);

    auto* lower = new QHBoxLayout();
    lower->setSpacing(10);

    auto* ips_box = new QGroupBox("Local IPs", central);
    auto* ips_layout = new QVBoxLayout(ips_box);
    local_ips_list_ = new QListWidget(ips_box);
    auto* ip_buttons = new QHBoxLayout();
    auto* refresh_ips_button = new QPushButton("Refresh", ips_box);
    auto* copy_ip_button = new QPushButton("Copy IP", ips_box);
    connect(refresh_ips_button, &QPushButton::clicked, this, [this] {
        refresh_ips();
    });
    connect(copy_ip_button, &QPushButton::clicked, this, [this] {
        copy_selected_ip();
    });
    ip_buttons->addWidget(refresh_ips_button);
    ip_buttons->addWidget(copy_ip_button);
    ips_layout->addWidget(local_ips_list_);
    ips_layout->addLayout(ip_buttons);
    lower->addWidget(ips_box);

    auto* devices_box = new QGroupBox("Devices", central);
    auto* devices_layout = new QVBoxLayout(devices_box);
    capture_devices_list_ = new QListWidget(devices_box);
    playback_devices_list_ = new QListWidget(devices_box);
    auto* refresh_devices_button = new QPushButton("Refresh Devices", devices_box);
    connect(refresh_devices_button, &QPushButton::clicked, this, [this] {
        refresh_devices();
    });
    devices_layout->addWidget(new QLabel("Capture", devices_box));
    devices_layout->addWidget(capture_devices_list_);
    devices_layout->addWidget(new QLabel("Playback", devices_box));
    devices_layout->addWidget(playback_devices_list_);
    devices_layout->addWidget(refresh_devices_button);
    lower->addWidget(devices_box);

    auto* diagnostics_box = new QGroupBox("Diagnostics", central);
    auto* diagnostics_layout = new QVBoxLayout(diagnostics_box);
    log_view_ = new QPlainTextEdit(diagnostics_box);
    log_view_->setReadOnly(true);
    auto* diagnostics_buttons = new QHBoxLayout();
    auto* copy_diagnostics_button = new QPushButton("Copy Diagnostics", diagnostics_box);
    auto* help_button = new QPushButton("Help", diagnostics_box);
    connect(copy_diagnostics_button, &QPushButton::clicked, this, [this] {
        copy_diagnostics();
    });
    connect(help_button, &QPushButton::clicked, this, [this] {
        show_help();
    });
    diagnostics_buttons->addWidget(copy_diagnostics_button);
    diagnostics_buttons->addWidget(help_button);
    diagnostics_layout->addWidget(log_view_);
    diagnostics_layout->addLayout(diagnostics_buttons);
    lower->addWidget(diagnostics_box);

    root->addLayout(lower);
    setCentralWidget(central);
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
    state_label_->setText(session_mode_label(status.mode));
    port_label_->setText(QString::number(status.port));
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

    start_share_button_->setEnabled(idle);
    connect_button_->setEnabled(idle);
    stop_button_->setEnabled(!idle);
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
    select_combo_data(capture_combo_, qstr(config.transmitter.capture_device_id));
    select_combo_data(playback_combo_, qstr(config.receiver.playback_device_id));
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
    if (selected == AudioMode::Quality) {
        show_error("Quality mode requires Opus implementation.");
        return;
    }
    const auto device_id = capture_combo_->currentData().toString().toStdString();
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
    const auto device_id = playback_combo_->currentData().toString().toStdString();
    auto result = controller_.start_listening(host.toStdString(), device_id);
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
        "Balanced is the default mode. Ultrafast uses smaller PCM packets.\n"
        "Listen connects to another machine and autodetects the stream mode from the SAL1 header.\n"
        "Quality/Opus, browser listening, and Android/Web compatibility are not available yet.");
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

} // namespace shareaudio::gui
