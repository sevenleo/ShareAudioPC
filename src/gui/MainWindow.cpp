#include "gui/MainWindow.h"
#include "gui/Theme.h"

#include "app/Config.h"
#include "app/StartupConfig.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QIcon>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QGroupBox>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QScreen>
#include <QShowEvent>
#include <QSlider>
#include <QSizePolicy>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#ifdef _WIN32
#include <dwmapi.h>
#include <windows.h>
#endif
#include <QVariant>
#include <QWidget>

#include <cmath>
#include <iomanip>
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
    switch (mode) {
    case AudioMode::Balanced:
        return "Balanced";
    case AudioMode::Fast:
        return "Fast";
    case AudioMode::Efficient:
        return "Efficient";
    }
    return qstr(to_string(mode));
}

QString detected_mode_label(AudioMode mode)
{
    switch (mode) {
    case AudioMode::Balanced:
        return "Balanced (PCM)";
    case AudioMode::Fast:
        return "Fast (PCM)";
    case AudioMode::Efficient:
        return "Efficient (Opus)";
    }
    return qstr(to_string(mode));
}

QString session_mode_label(SessionMode mode)
{
    switch (mode) {
    case SessionMode::Idle:
        return "Idle";
    case SessionMode::Sharing:
        return "Sharing";
    case SessionMode::Connecting:
        return "Connecting";
    case SessionMode::Listening:
        return "Listening";
    case SessionMode::SharingConnecting:
        return "Sharing + Connecting";
    case SessionMode::SharingListening:
        return "Sharing + Listening";
    }
    return "Idle";
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
    if (button->text() != text) {
        button->setText(text);
    }
    if (button->objectName() != object_name) {
        button->setObjectName(object_name);
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }
}

void update_visual_property(QWidget* widget, const char* property, const QVariant& value)
{
    if (widget == nullptr || widget->property(property) == value) {
        return;
    }

    widget->setProperty(property, value);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QLabel* make_caption(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("fieldCaption");
    return label;
}

QLabel* make_value(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("fieldValue");
    return label;
}

QLabel* make_panel_title(const QString& icon, const QString& text, const QString& tone, QWidget* parent)
{
    auto* label = new QLabel(icon + "  " + text, parent);
    label->setObjectName("panelTitle");
    label->setProperty("tone", tone);
    return label;
}

void add_status_item(QHBoxLayout* layout, const QString& caption, QLabel* value, QWidget* parent)
{
    auto* caption_label = new QLabel(caption, parent);
    caption_label->setObjectName("statusCaption");
    layout->addWidget(caption_label);
    layout->addWidget(value);
}

void move_grid_widget(QGridLayout* layout, QWidget* widget, int row, int column, int row_span = 1, int column_span = 1)
{
    if (layout == nullptr || widget == nullptr) {
        return;
    }

    layout->removeWidget(widget);
    layout->addWidget(widget, row, column, row_span, column_span);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , controller_(load_saved_app_config())
{
    build_ui();
    setStyleSheet(app_theme_stylesheet());
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

bool MainWindow::should_start_hidden() const
{
    return start_hidden_ && tray_available_;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!force_exit_ && tray_mode_ && tray_available_) {
        save_config_file(default_config_path(), controller_.config_snapshot());
        hide_to_tray();
        event->ignore();
        return;
    }

    save_config_file(default_config_path(), controller_.config_snapshot());
    controller_.stop();
    event->accept();
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);

    if (event->type() != QEvent::WindowStateChange) {
        return;
    }

    update_tray_actions();
    if (tray_mode_ && tray_available_ && isMinimized()) {
        QTimer::singleShot(0, this, [this] {
            if (tray_mode_ && tray_available_ && isMinimized()) {
                hide_to_tray();
            }
        });
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    apply_responsive_layout();
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
#ifdef _WIN32
    const BOOL enabled = FALSE;
    constexpr DWORD immersive_dark_mode_attribute = 20;
    DwmSetWindowAttribute(
        reinterpret_cast<HWND>(winId()),
        immersive_dark_mode_attribute,
        &enabled,
        sizeof(enabled));
#endif
    update_tray_actions();
}

void MainWindow::hideEvent(QHideEvent* event)
{
    QMainWindow::hideEvent(event);
    update_tray_actions();
}

void MainWindow::build_ui()
{
    setWindowTitle("ShareAudioPC");
    setWindowIcon(QIcon(":/icon/logo.png"));
    setMinimumSize(880, 560);

    auto* central = new QWidget(this);
    central->setObjectName("centralWorkspace");
    auto* main_root = new QVBoxLayout(central);
    main_root->setContentsMargins(0, 0, 0, 0);
    main_root->setSpacing(0);

    content_scroll_ = new QScrollArea(central);
    content_scroll_->setObjectName("contentScroll");
    content_scroll_->setWidgetResizable(true);
    content_scroll_->setFrameShape(QFrame::NoFrame);
    content_scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    content_widget_ = new QWidget(content_scroll_);
    content_widget_->setObjectName("contentWorkspace");
    auto* root = new QVBoxLayout(content_widget_);
    root->setContentsMargins(18, 14, 18, 16);
    root->setSpacing(16);

    status_panel_ = new QGroupBox(central);
    status_panel_->setObjectName("statusPanel");
    auto* status_layout = new QHBoxLayout(status_panel_);
    status_layout->setContentsMargins(18, 10, 18, 10);
    status_layout->setSpacing(14);

    auto* status_dot = new QLabel(QString::fromUtf8("\xE2\x97\x8F"), status_panel_);
    status_dot->setObjectName("statusDot");
    state_label_ = new QLabel("Ready", status_panel_);
    state_label_->setObjectName("stateValue");

    ip_info_label_ = new QLabel("-", status_panel_);
    ip_info_label_->setObjectName("addressValue");
    ip_info_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    port_label_ = new QLabel(QString::number(Defaults::tcp_port), status_panel_);
    port_label_->setObjectName("portValue");

    receiver_summary_label_ = new QLabel("-", status_panel_);
    receiver_summary_label_->setObjectName("addressValue");
    receiver_summary_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    auto* health_label = new QLabel("All good", status_panel_);
    health_label->setObjectName("healthValue");

    error_label_ = new QLabel("-", status_panel_);
    error_label_->setObjectName("errorValue");
    error_label_->setWordWrap(true);
    error_label_->setVisible(false);

    status_layout->addWidget(status_dot);
    add_status_item(status_layout, "Status:", state_label_, status_panel_);
    status_layout->addSpacing(18);
    add_status_item(status_layout, "Local IP:", ip_info_label_, status_panel_);
    status_layout->addSpacing(18);
    add_status_item(status_layout, "Port:", port_label_, status_panel_);
    status_layout->addSpacing(18);
    add_status_item(status_layout, "Receiver:", receiver_summary_label_, status_panel_);
    status_layout->addStretch(1);
    status_layout->addWidget(health_label);

    root->addWidget(status_panel_);
    root->addWidget(error_label_);

    primary_area_ = new QWidget(content_widget_);
    primary_area_->setObjectName("primarySessionArea");
    primary_layout_ = new QGridLayout(primary_area_);
    primary_layout_->setContentsMargins(0, 0, 0, 0);
    primary_layout_->setHorizontalSpacing(16);
    primary_layout_->setVerticalSpacing(16);

    sharing_panel_ = new QGroupBox(primary_area_);
    sharing_panel_->setObjectName("sharingPanel");
    sharing_panel_->setMinimumWidth(520);
    auto* server_layout = new QVBoxLayout(sharing_panel_);
    server_layout->setContentsMargins(24, 18, 24, 20);
    server_layout->setSpacing(16);

    auto* server_title_row = new QHBoxLayout();
    server_title_row->addWidget(make_panel_title("Audio", "SHARING (Transmitter)", "sharing", sharing_panel_));
    server_title_row->addStretch(1);
    clients_label_ = new QLabel("0 clients", sharing_panel_);
    clients_label_->setObjectName("clientsBadge");
    server_title_row->addWidget(clients_label_);
    server_layout->addLayout(server_title_row);

    start_share_button_ = new QPushButton("Start Sharing", sharing_panel_);
    start_share_button_->setObjectName("startShareButton");
    start_share_button_->setMinimumHeight(48);
    start_share_button_->setMinimumWidth(260);
    start_share_button_->setMaximumWidth(320);
    start_share_button_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connect(start_share_button_, &QPushButton::clicked, this, [this] {
        const auto status = controller_.status_snapshot();
        if (status.sharing_active) {
            stop_sharing();
        } else {
            start_sharing();
        }
    });
    server_layout->addWidget(start_share_button_, 0, Qt::AlignLeft);

    auto* share_summary = new QGridLayout();
    share_summary->setHorizontalSpacing(28);
    share_summary->setVerticalSpacing(12);
    share_summary->addWidget(make_caption("Mode:", sharing_panel_), 0, 0);
    share_mode_summary_label_ = make_value("-", sharing_panel_);
    share_mode_summary_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    share_summary->addWidget(share_mode_summary_label_, 0, 1);
    share_summary->addWidget(make_caption("Device:", sharing_panel_), 1, 0);
    capture_summary_label_ = make_value("-", sharing_panel_);
    capture_summary_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    share_summary->addWidget(capture_summary_label_, 1, 1);
    share_summary->addWidget(make_caption("Clients:", sharing_panel_), 2, 0);
    simple_clients_label_ = make_value("0", sharing_panel_);
    share_summary->addWidget(simple_clients_label_, 2, 1);
    server_layout->addLayout(share_summary);

#ifdef _WIN32
    follow_system_volume_checkbox_ = new QCheckBox("Follow system volume (Windows only)", sharing_panel_);
    follow_system_volume_checkbox_->setObjectName("followSystemVolumeCheckbox");
    follow_system_volume_checkbox_->setToolTip(
        "Scale transmitted audio using the selected Windows output device master volume.");
    connect(follow_system_volume_checkbox_, &QCheckBox::toggled, this, [this](bool checked) {
        auto result = controller_.set_volume_mode(checked ? VolumeMode::System : VolumeMode::Full);
        if (!result.ok()) {
            const QSignalBlocker blocker(follow_system_volume_checkbox_);
            follow_system_volume_checkbox_->setChecked(!checked);
            return;
        }
        save_config_file(default_config_path(), controller_.config_snapshot());
    });
    server_layout->addWidget(follow_system_volume_checkbox_);
#endif

    server_advanced_widget_ = new QWidget(sharing_panel_);
    auto* server_adv_layout = new QGridLayout(server_advanced_widget_);
    server_adv_layout->setContentsMargins(0, 0, 0, 0);
    server_adv_layout->setHorizontalSpacing(12);
    server_adv_layout->setVerticalSpacing(12);
    server_adv_layout->setColumnMinimumWidth(0, 118);
    server_adv_layout->setColumnStretch(1, 1);

    mode_combo_ = new QComboBox(server_advanced_widget_);
    mode_combo_->setMinimumWidth(220);
    mode_combo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mode_combo_->addItem("Balanced (Recommended)", QVariant::fromValue(static_cast<int>(AudioMode::Balanced)));
    mode_combo_->addItem("Fast (Low Latency)", QVariant::fromValue(static_cast<int>(AudioMode::Fast)));
    mode_combo_->addItem("Efficient (Low Data)", QVariant::fromValue(static_cast<int>(AudioMode::Efficient)));

    capture_combo_ = new QComboBox(server_advanced_widget_);
    capture_combo_->setMinimumWidth(220);
    capture_combo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* refresh_capture_button = new QPushButton(server_advanced_widget_);
    refresh_capture_button->setObjectName("iconButton");
    refresh_capture_button->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    refresh_capture_button->setToolTip("Refresh audio devices");
    connect(refresh_capture_button, &QPushButton::clicked, this, [this] {
        refresh_devices();
    });

    volume_gain_label_ = new QLabel("Current system volume: -", server_advanced_widget_);
    volume_gain_label_->setObjectName("linkValue");
    volume_slider_ = new QSlider(Qt::Horizontal, server_advanced_widget_);
    volume_slider_->setObjectName("volumeSlider");
    volume_slider_->setRange(0, 100);
    volume_slider_->setEnabled(false);

    server_adv_layout->addWidget(make_caption("Audio Mode:", server_advanced_widget_), 0, 0);
    server_adv_layout->addWidget(mode_combo_, 0, 1, 1, 2);
    server_adv_layout->addWidget(make_caption("Capture Device:", server_advanced_widget_), 1, 0);
    server_adv_layout->addWidget(capture_combo_, 1, 1);
    server_adv_layout->addWidget(refresh_capture_button, 1, 2);
    server_adv_layout->addWidget(make_caption("Volume Mode:", server_advanced_widget_), 2, 0);
    server_adv_layout->addWidget(volume_gain_label_, 2, 1, 1, 2);
    server_adv_layout->addWidget(volume_slider_, 3, 1, 1, 2);
    server_layout->addWidget(server_advanced_widget_);
    server_layout->addStretch(1);

    receiver_panel_ = new QGroupBox(primary_area_);
    receiver_panel_->setObjectName("receiverPanel");
    receiver_panel_->setMinimumWidth(520);
    auto* listen_layout = new QVBoxLayout(receiver_panel_);
    listen_layout->setContentsMargins(24, 18, 24, 20);
    listen_layout->setSpacing(16);

    auto* receiver_title_row = new QHBoxLayout();
    receiver_title_row->addWidget(make_panel_title("Input", "RECEIVER", "receiver", receiver_panel_));
    receiver_title_row->addStretch(1);
    listen_mode_label_ = new QLabel("-", receiver_panel_);
    listen_mode_label_->setObjectName("modeBadge");
    receiver_title_row->addWidget(new QLabel("Detected mode:", receiver_panel_));
    receiver_title_row->addWidget(listen_mode_label_);
    listen_layout->addLayout(receiver_title_row);

    connect_button_ = new QPushButton("Connect Receiver", receiver_panel_);
    connect_button_->setObjectName("connectButton");
    connect_button_->setMinimumHeight(48);
    connect_button_->setMinimumWidth(260);
    connect_button_->setMaximumWidth(320);
    connect_button_->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    connect(connect_button_, &QPushButton::clicked, this, [this] {
        const auto status = controller_.status_snapshot();
        if (status.receiver_listening || status.receiver_connecting) {
            stop_listening();
        } else {
            start_listening();
        }
    });
    listen_layout->addWidget(connect_button_, 0, Qt::AlignLeft);

    host_input_ = new QLineEdit(receiver_panel_);
    host_input_->setPlaceholderText("Enter transmitter IP or hostname");
    host_input_->setMinimumHeight(40);
    host_input_->setMinimumWidth(220);
    host_input_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* receiver_simple_form = new QGridLayout();
    receiver_simple_form->setHorizontalSpacing(28);
    receiver_simple_form->setVerticalSpacing(12);
    receiver_simple_form->setColumnMinimumWidth(0, 118);
    receiver_simple_form->setColumnStretch(1, 1);
    receiver_simple_form->addWidget(make_caption("Transmitter IP:", receiver_panel_), 0, 0);
    receiver_simple_form->addWidget(host_input_, 0, 1);
    receiver_status_label_ = make_value("Idle", receiver_panel_);
    receiver_simple_form->addWidget(make_caption("Status:", receiver_panel_), 1, 0);
    receiver_simple_form->addWidget(receiver_status_label_, 1, 1);
    playback_summary_label_ = make_value("-", receiver_panel_);
    playback_summary_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    receiver_simple_form->addWidget(make_caption("Device:", receiver_panel_), 2, 0);
    receiver_simple_form->addWidget(playback_summary_label_, 2, 1);
    listen_layout->addLayout(receiver_simple_form);

    client_advanced_widget_ = new QWidget(receiver_panel_);
    auto* client_adv_layout = new QGridLayout(client_advanced_widget_);
    client_adv_layout->setContentsMargins(0, 0, 0, 0);
    client_adv_layout->setHorizontalSpacing(12);
    client_adv_layout->setVerticalSpacing(12);
    client_adv_layout->setColumnMinimumWidth(0, 118);
    client_adv_layout->setColumnStretch(1, 1);

    playback_combo_ = new QComboBox(client_advanced_widget_);
    playback_combo_->setMinimumWidth(220);
    playback_combo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* refresh_playback_button = new QPushButton(client_advanced_widget_);
    refresh_playback_button->setObjectName("iconButton");
    refresh_playback_button->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    refresh_playback_button->setToolTip("Refresh audio devices");
    connect(refresh_playback_button, &QPushButton::clicked, this, [this] {
        refresh_devices();
    });

    client_adv_layout->addWidget(make_caption("Playback Device:", client_advanced_widget_), 0, 0);
    client_adv_layout->addWidget(playback_combo_, 0, 1);
    client_adv_layout->addWidget(refresh_playback_button, 0, 2);
    listen_layout->addWidget(client_advanced_widget_);
    listen_layout->addStretch(1);

    primary_layout_->addWidget(sharing_panel_, 0, 0);
    primary_layout_->addWidget(receiver_panel_, 0, 1);
    primary_layout_->setColumnStretch(0, 1);
    primary_layout_->setColumnStretch(1, 1);
    root->addWidget(primary_area_);

    tabs_ = new QTabWidget(central);
    tabs_->setObjectName("mainTabs");

    // Tab 1: Network & History
    auto* dev_tab = new QWidget(tabs_);
    network_hardware_layout_ = new QGridLayout(dev_tab);
    network_hardware_layout_->setHorizontalSpacing(12);
    network_hardware_layout_->setVerticalSpacing(12);
    network_hardware_layout_->setContentsMargins(16, 16, 16, 16);

    network_panel_ = new QGroupBox("Local IP Addresses", dev_tab);
    network_panel_->setObjectName("subPanel");
    network_panel_->setMinimumWidth(320);
    auto* net_layout = new QVBoxLayout(network_panel_);
    local_ips_list_ = new QListWidget(network_panel_);
    recent_devices_list_ = new QListWidget(network_panel_);
    connect(recent_devices_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            host_input_->setText(item->text());
        }
    });

    auto* ip_buttons = new QHBoxLayout();
    auto* refresh_ips_button = new QPushButton("Refresh", network_panel_);
    refresh_ips_button->setObjectName("secondaryButton");
    auto* copy_ip_button = new QPushButton("Copy Selected IP", network_panel_);
    copy_ip_button->setObjectName("secondaryButton");
    connect(refresh_ips_button, &QPushButton::clicked, this, [this] {
        refresh_ips();
    });
    connect(copy_ip_button, &QPushButton::clicked, this, [this] {
        copy_selected_ip();
    });
    ip_buttons->addWidget(refresh_ips_button);
    ip_buttons->addWidget(copy_ip_button);

    net_layout->addWidget(local_ips_list_);
    net_layout->addLayout(ip_buttons);
    auto* recent_label = new QLabel("Recent Transmitters", network_panel_);
    recent_label->setObjectName("sectionLabel");
    net_layout->addWidget(recent_label);
    net_layout->addWidget(recent_devices_list_);

    devices_panel_ = new QGroupBox("Audio Devices", dev_tab);
    devices_panel_->setObjectName("subPanel");
    devices_panel_->setMinimumWidth(360);
    auto* devices_layout = new QVBoxLayout(devices_panel_);
    capture_devices_list_ = new QListWidget(devices_panel_);
    playback_devices_list_ = new QListWidget(devices_panel_);
    auto* refresh_devices_button = new QPushButton("Refresh Devices List", devices_panel_);
    refresh_devices_button->setObjectName("secondaryButton");
    connect(refresh_devices_button, &QPushButton::clicked, this, [this] {
        refresh_devices();
    });
    devices_layout->addWidget(new QLabel("Available Input/Capture Sources:", devices_panel_));
    devices_layout->addWidget(capture_devices_list_);
    devices_layout->addWidget(new QLabel("Available Output/Playback Speakers:", devices_panel_));
    devices_layout->addWidget(playback_devices_list_);
    devices_layout->addWidget(refresh_devices_button);

    quick_actions_panel_ = new QGroupBox("Quick Actions", dev_tab);
    quick_actions_panel_->setObjectName("subPanel");
    quick_actions_panel_->setMinimumWidth(260);
    auto* quick_layout = new QGridLayout(quick_actions_panel_);
    quick_layout->setSpacing(12);
    auto* quick_refresh = new QPushButton("Refresh Devices", quick_actions_panel_);
    quick_refresh->setObjectName("quickActionButton");
    quick_refresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(quick_refresh, &QPushButton::clicked, this, [this] {
        refresh_devices();
    });
    auto* quick_network = new QPushButton("Network Info", quick_actions_panel_);
    quick_network->setObjectName("quickActionButton");
    quick_network->setIcon(style()->standardIcon(QStyle::SP_DriveNetIcon));
    connect(quick_network, &QPushButton::clicked, this, [this] {
        refresh_ips();
    });
    auto* quick_copy = new QPushButton("Copy Local IP", quick_actions_panel_);
    quick_copy->setObjectName("quickActionButton");
    quick_copy->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(quick_copy, &QPushButton::clicked, this, [this] {
        copy_selected_ip();
    });
    auto* quick_help = new QPushButton("Help", quick_actions_panel_);
    quick_help->setObjectName("quickActionButton");
    quick_help->setIcon(style()->standardIcon(QStyle::SP_MessageBoxQuestion));
    connect(quick_help, &QPushButton::clicked, this, [this] {
        show_help();
    });
    quick_layout->addWidget(quick_refresh, 0, 0);
    quick_layout->addWidget(quick_network, 0, 1);
    quick_layout->addWidget(quick_copy, 1, 0);
    quick_layout->addWidget(quick_help, 1, 1);
    network_hardware_layout_->addWidget(network_panel_, 0, 0);
    network_hardware_layout_->addWidget(devices_panel_, 0, 1);
    network_hardware_layout_->addWidget(quick_actions_panel_, 0, 2);
    network_hardware_layout_->setColumnStretch(0, 1);
    network_hardware_layout_->setColumnStretch(1, 1);
    network_hardware_layout_->setColumnStretch(2, 0);

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
    copy_diag_button->setObjectName("secondaryButton");
    auto* help_button = new QPushButton("Help Guide", diag_tab);
    help_button->setObjectName("secondaryButton");
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

    auto* general_tab = new QWidget(tabs_);
    auto* general_layout = new QVBoxLayout(general_tab);
    general_layout->setContentsMargins(16, 16, 16, 16);
    auto* general_box = new QGroupBox("General", general_tab);
    general_box->setObjectName("subPanel");
    auto* general_form = new QFormLayout(general_box);
    general_form->addRow("Application:", new QLabel("ShareAudioPC", general_box));
    general_form->addRow("Version:", new QLabel(SHAREAUDIO_VERSION, general_box));
    general_form->addRow("Default port:", new QLabel(QString::number(Defaults::tcp_port), general_box));
    general_layout->addWidget(general_box);
    general_layout->addStretch(1);
    tabs_->addTab(general_tab, "General");

    root->addWidget(tabs_);
    content_scroll_->setWidget(content_widget_);
    main_root->addWidget(content_scroll_, 1);

    auto* footer_widget = new QWidget(central);
    footer_widget->setObjectName("footerBar");
    auto* footer_layout = new QHBoxLayout(footer_widget);
    footer_layout->setContentsMargins(18, 12, 18, 12);
    footer_layout->setSpacing(14);
    auto* footer_version = new QLabel(QStringLiteral("ShareAudioPC v") + SHAREAUDIO_VERSION, footer_widget);
    footer_version->setObjectName("footerVersion");

    tray_mode_checkbox_ = new QCheckBox("Minimize to tray", footer_widget);
    tray_mode_checkbox_->setObjectName("trayModeCheckbox");
    tray_mode_checkbox_->setToolTip("Hide the window in the system tray when minimized or closed.");
    connect(tray_mode_checkbox_, &QCheckBox::toggled, this, [this](bool checked) {
        set_tray_mode(checked);
    });
    footer_layout->addWidget(tray_mode_checkbox_);
    footer_layout->addStretch(1);
    footer_layout->addWidget(footer_version);
    toggle_mode_button_ = new QPushButton("Advanced", footer_widget);
    toggle_mode_button_->setObjectName("secondaryButton");
    toggle_mode_button_->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(toggle_mode_button_, &QPushButton::clicked, this, [this] {
        advanced_mode_ = !advanced_mode_;
        update_layout_visibility();
    });
    footer_layout->addWidget(toggle_mode_button_);
    help_footer_button_ = new QPushButton("Help", footer_widget);
    help_footer_button_->setObjectName("secondaryButton");
    help_footer_button_->setIcon(style()->standardIcon(QStyle::SP_MessageBoxQuestion));
    connect(help_footer_button_, &QPushButton::clicked, this, [this] {
        show_help();
    });
    footer_layout->addWidget(help_footer_button_);
    main_root->addWidget(footer_widget);

    setCentralWidget(central);

    setup_tray();
    update_layout_visibility();
}

void MainWindow::setup_tray()
{
    tray_available_ = QSystemTrayIcon::isSystemTrayAvailable();
    if (!tray_available_) {
        if (tray_mode_checkbox_) {
            tray_mode_checkbox_->setEnabled(false);
            tray_mode_checkbox_->setToolTip("System tray is not available.");
        }
        return;
    }

    tray_icon_ = new QSystemTrayIcon(QIcon(":/icon/logo.png"), this);
    tray_icon_->setToolTip("ShareAudioPC");

    tray_menu_ = new QMenu(this);
    toggle_window_action_ = tray_menu_->addAction("Show Window");
    connect(toggle_window_action_, &QAction::triggered, this, [this] {
        toggle_window_visibility();
    });

    tray_mode_action_ = tray_menu_->addAction("Minimize to tray");
    tray_mode_action_->setCheckable(true);
    connect(tray_mode_action_, &QAction::toggled, this, [this](bool checked) {
        set_tray_mode(checked);
    });

    tray_menu_->addSeparator();
    exit_action_ = tray_menu_->addAction("Exit");
    connect(exit_action_, &QAction::triggered, this, [this] {
        exit_from_tray();
    });

    tray_icon_->setContextMenu(tray_menu_);
    connect(tray_icon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            show_from_tray();
        }
    });

    tray_icon_->show();
    update_tray_actions();
}

void MainWindow::set_tray_mode(bool enabled)
{
    tray_mode_ = enabled && tray_available_;

    if (tray_mode_checkbox_ && tray_mode_checkbox_->isChecked() != tray_mode_) {
        const QSignalBlocker blocker(tray_mode_checkbox_);
        tray_mode_checkbox_->setChecked(tray_mode_);
    }
    if (tray_mode_action_ && tray_mode_action_->isChecked() != tray_mode_) {
        const QSignalBlocker blocker(tray_mode_action_);
        tray_mode_action_->setChecked(tray_mode_);
    }

    update_tray_actions();
}

void MainWindow::show_from_tray()
{
    showNormal();
    raise();
    activateWindow();
    update_tray_actions();
}

void MainWindow::hide_to_tray()
{
    if (!tray_available_) {
        return;
    }

    hide();
    update_tray_actions();
}

void MainWindow::toggle_window_visibility()
{
    if (isVisible() && !isMinimized()) {
        hide_to_tray();
    } else {
        show_from_tray();
    }
}

void MainWindow::exit_from_tray()
{
    force_exit_ = true;
    close();
    QApplication::quit();
}

void MainWindow::update_tray_actions()
{
    if (!tray_available_) {
        return;
    }

    if (toggle_window_action_) {
        toggle_window_action_->setText(isVisible() && !isMinimized() ? "Hide Window" : "Show Window");
    }
    if (tray_icon_) {
        const auto status = controller_.status_snapshot();
        tray_icon_->setToolTip(QStringLiteral("ShareAudioPC - ") + session_mode_label(status.mode));
    }
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

    auto resize_for_mode = [this](const QSize& desired, const QSize& desired_minimum) {
        QScreen* active_screen = screen();
        if (active_screen == nullptr) {
            active_screen = QGuiApplication::primaryScreen();
        }

        QSize maximum = desired;
        if (active_screen != nullptr) {
            const QSize available = active_screen->availableGeometry().size() - QSize(32, 32);
            maximum = QSize(qMax(640, available.width()), qMax(480, available.height()));
        }

        const QSize actual = desired.boundedTo(maximum);
        const QSize minimum(
            qMin(desired_minimum.width(), actual.width()),
            qMin(desired_minimum.height(), actual.height()));
        setMinimumSize(minimum);
        setMaximumSize(16777215, 16777215);
        resize(actual);
    };

    if (advanced_mode_) {
        setWindowTitle("ShareAudioPC - Advanced Mode");
        resize_for_mode(QSize(1366, 900), QSize(1180, 760));
        if (toggle_mode_button_) {
            toggle_mode_button_->setText("Simple Mode");
            toggle_mode_button_->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
        }
    } else {
        setWindowTitle("ShareAudioPC");
        resize_for_mode(QSize(1086, 660), QSize(920, 620));
        if (toggle_mode_button_) {
            toggle_mode_button_->setText("Advanced");
            toggle_mode_button_->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        }
    }
    apply_responsive_layout();
}

void MainWindow::apply_responsive_layout()
{
    if (primary_layout_ == nullptr) {
        return;
    }

    const int available_width = content_scroll_ != nullptr && content_scroll_->viewport() != nullptr
        ? content_scroll_->viewport()->width()
        : width();
    const bool compact_primary = available_width < 1120;

    if (compact_primary) {
        sharing_panel_->setMinimumWidth(0);
        receiver_panel_->setMinimumWidth(0);
        move_grid_widget(primary_layout_, sharing_panel_, 0, 0);
        move_grid_widget(primary_layout_, receiver_panel_, 1, 0);
        primary_layout_->setColumnStretch(0, 1);
        primary_layout_->setColumnStretch(1, 0);
        primary_layout_->setRowStretch(0, 0);
        primary_layout_->setRowStretch(1, 0);
    } else {
        sharing_panel_->setMinimumWidth(520);
        receiver_panel_->setMinimumWidth(520);
        move_grid_widget(primary_layout_, sharing_panel_, 0, 0);
        move_grid_widget(primary_layout_, receiver_panel_, 0, 1);
        primary_layout_->setColumnStretch(0, 1);
        primary_layout_->setColumnStretch(1, 1);
        primary_layout_->setRowStretch(0, 0);
        primary_layout_->setRowStretch(1, 0);
    }

    if (network_hardware_layout_ != nullptr) {
        const bool compact_tabs = available_width < 1180;
        if (compact_tabs) {
            move_grid_widget(network_hardware_layout_, network_panel_, 0, 0);
            move_grid_widget(network_hardware_layout_, devices_panel_, 1, 0);
            move_grid_widget(network_hardware_layout_, quick_actions_panel_, 2, 0);
            network_hardware_layout_->setColumnStretch(0, 1);
            network_hardware_layout_->setColumnStretch(1, 0);
            network_hardware_layout_->setColumnStretch(2, 0);
        } else {
            move_grid_widget(network_hardware_layout_, network_panel_, 0, 0);
            move_grid_widget(network_hardware_layout_, devices_panel_, 0, 1);
            move_grid_widget(network_hardware_layout_, quick_actions_panel_, 0, 2);
            network_hardware_layout_->setColumnStretch(0, 1);
            network_hardware_layout_->setColumnStretch(1, 1);
            network_hardware_layout_->setColumnStretch(2, 0);
        }
    }
}

void MainWindow::refresh_all()
{
    refresh_ips();
    refresh_devices();
    const auto config = controller_.config_snapshot();
    select_combo_data(mode_combo_, QVariant::fromValue(static_cast<int>(config.transmitter.mode)));
    if (follow_system_volume_checkbox_) {
        const QSignalBlocker blocker(follow_system_volume_checkbox_);
        follow_system_volume_checkbox_->setChecked(config.transmitter.volume_mode == VolumeMode::System);
    }
    if (!config.receiver.host.empty()) {
        host_input_->setText(qstr(config.receiver.host));
    }
    refresh_recent_devices();
    refresh_status();
}

void MainWindow::refresh_status()
{
    const auto status = controller_.status_snapshot();
    const bool sharing = status.sharing_active;
    const bool listening = status.receiver_listening || status.receiver_connecting;

    QString state_tone = "idle";
    if (sharing && listening) {
        state_tone = "combined";
    } else if (sharing) {
        state_tone = "sharing";
    } else if (status.receiver_connecting) {
        state_tone = "connecting";
    } else if (status.receiver_listening) {
        state_tone = "listening";
    } else if (!status.last_error.empty()) {
        state_tone = "error";
    }
    const QString receiver_state = status.receiver_connecting ? "connecting"
        : (status.receiver_listening ? "active" : "idle");
    const bool has_error = !status.last_error.empty();

    update_visual_property(status_panel_, "stateTone", state_tone);
    update_visual_property(state_label_, "tone", state_tone);
    update_visual_property(sharing_panel_, "active", sharing);
    update_visual_property(receiver_panel_, "state", receiver_state);
    update_visual_property(error_label_, "hasError", has_error);
    error_label_->setVisible(has_error);

    state_label_->setText(session_mode_label(status.mode));
    port_label_->setText(QString::number(status.port));
    
    // Format IP info label depending on the mode initiated
    auto local_ip_summary = [this]() {
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
        return ips_str;
    };

    ip_info_label_->setText(local_ip_summary());
    if (listening) {
        const QString suffix = status.receiver_connecting ? " (Connecting)" : " (Listening)";
        receiver_summary_label_->setText(qstr(status.host) + suffix);
    } else {
        receiver_summary_label_->setText("-");
    }

    error_label_->setText(status.last_error.empty() ? "-" : qstr(status.last_error));
    clients_label_->setText(QString("%1 client%2 connected")
                                .arg(status.connected_clients)
                                .arg(status.connected_clients == 1 ? "" : "s"));
    simple_clients_label_->setText(QString::number(status.connected_clients));
    share_mode_summary_label_->setText(mode_label(status.selected_mode));
    const QString capture_text = capture_combo_->currentText();
    capture_summary_label_->setText(capture_text.isEmpty() ? "-" : capture_text);
    const QString playback_text = playback_combo_->currentText();
    playback_summary_label_->setText(playback_text.isEmpty() ? "-" : playback_text);
    bytes_sent_label_->setText(QString::number(status.bytes_sent));
    packets_label_->setText(QString::number(status.packets_produced));
    dropped_label_->setText(QString::number(status.dropped_packets));
    listen_mode_label_->setText(status.has_detected_mode ? detected_mode_label(status.detected_mode) : "-");
    bytes_received_label_->setText(QString::number(status.bytes_received));
    bytes_played_label_->setText(QString::number(status.bytes_played));
    buffer_label_->setText(QString::number(status.jitter_buffer_depth));
    underruns_label_->setText(QString::number(status.underruns));
    receiver_status_label_->setText(status.receiver_connecting ? "Connecting" : (status.receiver_listening ? "Listening" : "Idle"));
    update_visual_property(receiver_status_label_, "tone", receiver_state);
    update_visual_property(listen_mode_label_, "active", status.has_detected_mode);
    if (volume_gain_label_ && volume_slider_) {
        const int gain_percent = qBound(0, static_cast<int>(std::round(status.system_volume_gain * 100.0)), 100);
        volume_gain_label_->setText(QString("Current system volume: %1%").arg(gain_percent));
        volume_slider_->setValue(gain_percent);
    }

    // Server button toggle state and text
    if (sharing) {
        update_button_style(start_share_button_, "stopButton", "Stop Sharing");
        start_share_button_->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    } else {
        update_button_style(start_share_button_, "startShareButton", "Start Sharing");
        start_share_button_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }

    // Client button toggle state and text
    if (listening) {
        update_button_style(connect_button_, "stopButton", "Disconnect");
        connect_button_->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
    } else {
        update_button_style(connect_button_, "connectButton", "Connect Receiver");
        connect_button_->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    }

    start_share_button_->setEnabled(true);
    connect_button_->setEnabled(true);
    mode_combo_->setEnabled(!sharing);
    capture_combo_->setEnabled(!sharing);
    if (follow_system_volume_checkbox_) {
        follow_system_volume_checkbox_->setEnabled(!sharing);
    }
    playback_combo_->setEnabled(!listening);
    host_input_->setEnabled(!listening);

    QString logs;
    for (const auto& event : status.log_events) {
        logs += qstr(event) + "\n";
    }
    if (log_view_->toPlainText() != logs) {
        log_view_->setPlainText(logs);
    }
    refresh_recent_devices();
    update_tray_actions();
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
    const VolumeMode volume_mode = follow_system_volume_checkbox_ != nullptr
        ? (follow_system_volume_checkbox_->isChecked() ? VolumeMode::System : VolumeMode::Full)
        : controller_.config_snapshot().transmitter.volume_mode;
    auto result = controller_.start_sharing(selected, device_id, volume_mode);
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

void MainWindow::stop_sharing()
{
    controller_.stop_sharing();
    save_config_file(default_config_path(), controller_.config_snapshot());
    refresh_status();
}

void MainWindow::stop_listening()
{
    controller_.stop_listening();
    save_config_file(default_config_path(), controller_.config_snapshot());
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
    const QString message = QString("Share starts a transmitter on TCP port %1.\n"
                                     "Balanced is the default AudioMode. Fast uses smaller PCM packets.\n"
                                     "Efficient uses Opus when this build is linked with libopus.\n"
                                     "Follow system volume applies the selected Windows output-device master volume to transmitted audio.\n"
                                     "Listen connects to another machine and autodetects the stream mode from SAL1 or HTTP metadata.\n"
                                     "Browser/mobile compatibility is exposed through /info, /stream, and the browser player at http://<IP>:%1/.")
                                 .arg(Defaults::tcp_port);
    QMessageBox::information(
        this,
        "ShareAudioPC Help",
        message);
}

void MainWindow::show_error(const QString& message)
{
    error_label_->setText(message);
    QMessageBox::warning(this, "ShareAudioPC", message);
}

QString MainWindow::diagnostics_text() const
{
    const auto status = controller_.status_snapshot();
    std::ostringstream out;
    out << "ShareAudioPC " << SHAREAUDIO_VERSION << "\n";
    out << "state=" << to_string(status.mode) << "\n";
    out << "sharing_active=" << (status.sharing_active ? "true" : "false") << "\n";
    out << "receiver_connecting=" << (status.receiver_connecting ? "true" : "false") << "\n";
    out << "receiver_listening=" << (status.receiver_listening ? "true" : "false") << "\n";
    out << "port=" << status.port << "\n";
    out << "selected_mode=" << to_string(status.selected_mode) << "\n";
    out << "volume_mode=" << to_string(status.volume_mode) << "\n";
    out << "system_volume_gain=" << std::fixed << std::setprecision(4) << status.system_volume_gain << "\n";
    out << "system_volume_tracking=" << to_string(status.system_volume_tracking) << "\n";
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

    set_tray_mode(cfg.traymode || cfg.startintray);
    start_hidden_ = cfg.startintray && tray_mode_ && tray_available_;

    // Pre-fill GUI fields regardless of AUTOSTART
    if (cfg.has_audio_mode()) {
        auto parsed = cfg.parsed_audio_mode();
        if (parsed) {
            select_combo_data(mode_combo_, QVariant::fromValue(static_cast<int>(*parsed)));
        }
    }
    if (cfg.has_volume_mode()) {
        auto parsed = cfg.parsed_volume_mode();
        if (parsed) {
            (void)controller_.set_volume_mode(*parsed);
            if (follow_system_volume_checkbox_) {
                const QSignalBlocker blocker(follow_system_volume_checkbox_);
                follow_system_volume_checkbox_->setChecked(*parsed == VolumeMode::System);
            }
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

    if ((cfg.is_client() || cfg.is_both()) && !cfg.has_server_ip()) {
        // Client/both mode without SERVER_IP — skip autostart, open normally
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
    } else if (cfg.is_both()) {
        QTimer::singleShot(200, this, [this] {
            start_sharing();
            start_listening();
        });
    }
}

} // namespace shareaudio::gui
