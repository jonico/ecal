/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

#include "Ecalmon.h"

#include "ecal/ecal.h"

#include "Widgets/AboutDialog/AboutDialog.h"
#include "Widgets/LicenseDialog/LicenseDialog.h"
#include "Widgets/PluginSettingsDialog/PluginSettingsDialog.h"

#ifdef ECAL_NPCAP_SUPPORT
#include "Widgets/NpcapStatusDialog/NpcapStatusDialog.h"
#endif //ECAL_NPCAP_SUPPORT

#include "EcalmonGlobals.h"
#include "PluginLoader.h"

#include <QSettings>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDateTime>

#include <QDebug>

#include <iomanip>
#include <string>
#include <ctime>
#include <chrono>
#include <sstream>

Ecalmon::Ecalmon(QWidget *parent)
  : QMainWindow(parent)
  , monitor_error_counter_(0)
{
  // Just make sure that eCAL is initialized
  eCAL::Initialize(0, nullptr, "eCALMon", eCAL::Init::Default | eCAL::Init::Monitoring);
  eCAL::Monitoring::SetFilterState(false);
  eCAL::Process::SetState(proc_sev_healthy, proc_sev_level1, "Running");

  ui_.setupUi(this);

  // Setup Status bar
  error_label_                = new QLabel(this);
  error_label_                ->setHidden(true);
  error_label_                ->setStyleSheet("background-color: rgb(255, 128, 128);");

  monitor_update_speed_label_ = new QLabel(this);
  log_update_speed_label_     = new QLabel(this);
  time_label_                 = new QLabel(this);

  monitor_update_speed_label_->setMinimumWidth(10);
  log_update_speed_label_    ->setMinimumWidth(10);
  time_label_                ->setMinimumWidth(10);
  error_label_               ->setMinimumWidth(10);

  ui_.statusbar->addWidget(monitor_update_speed_label_);
  ui_.statusbar->addWidget(log_update_speed_label_);
  ui_.statusbar->addWidget(time_label_);
  ui_.statusbar->addWidget(error_label_);

  ecal_time_update_timer_ = new QTimer(this);
  connect(ecal_time_update_timer_, &QTimer::timeout, this, &Ecalmon::updateEcalTime);
  ecal_time_update_timer_->start(100);

  ui_.central_widget->hide(); // We must always have a central widget. But we can hide it, because we only want to display stuff in DockWidgets

  tabifyDockWidget(ui_.topics_dockwidget, ui_.processes_dockwidget);
  tabifyDockWidget(ui_.topics_dockwidget, ui_.host_dockwidget);
  tabifyDockWidget(ui_.topics_dockwidget, ui_.service_dockwidget);
  ui_.topics_dockwidget->raise();

  log_widget_               = new LogWidget(this);
  topic_widget_             = new TopicWidget(this);
  process_widget_           = new ProcessWidget(this);
  host_widget_              = new HostWidget(this);
  service_widget_           = new ServiceWidget(this);
  syste_information_widget_ = new SystemInformationWidget(this);

  ui_.logging_dockwidget_content_frame_layout           ->addWidget(log_widget_);
  ui_.topics_dockwidget_content_frame_layout            ->addWidget(topic_widget_);
  ui_.processes_dockwidget_content_frame_layout         ->addWidget(process_widget_);
  ui_.host_dockwidget_content_frame_layout              ->addWidget(host_widget_);
  ui_.service_dockwidget_content_frame_layout           ->addWidget(service_widget_);
  ui_.system_information_dockwidget_content_frame_layout->addWidget(syste_information_widget_);

  monitor_update_timer_ = new QTimer(this);
  connect(monitor_update_timer_, &QTimer::timeout, [this](){updateMonitor();});
  monitor_update_timer_->start(1000);


  connect(this, &Ecalmon::monitorUpdatedSignal, [this](eCAL::pb::Monitoring monitoring_pb) {topic_widget_  ->monitorUpdated(monitoring_pb);});
  connect(this, &Ecalmon::monitorUpdatedSignal, [this](eCAL::pb::Monitoring monitoring_pb) {process_widget_->monitorUpdated(monitoring_pb); });
  connect(this, &Ecalmon::monitorUpdatedSignal, [this](eCAL::pb::Monitoring monitoring_pb) {host_widget_   ->monitorUpdated(monitoring_pb); });
  connect(this, &Ecalmon::monitorUpdatedSignal, [this](eCAL::pb::Monitoring monitoring_pb) {service_widget_->monitorUpdated(monitoring_pb); });

  // Monitor Update Speed selection
  monitor_update_speed_group_ = new QActionGroup(this);
  monitor_update_speed_group_->addAction(ui_.action_monitor_refresh_speed_0_5s);
  monitor_update_speed_group_->addAction(ui_.action_monitor_refresh_speed_1s);
  monitor_update_speed_group_->addAction(ui_.action_monitor_refresh_speed_2s);
  monitor_update_speed_group_->addAction(ui_.action_monitor_refresh_speed_5s);
  monitor_update_speed_group_->addAction(ui_.action_monitor_refresh_speed_10s);

  connect(monitor_update_speed_group_,             &QActionGroup::triggered, this, &Ecalmon::updateMonitorUpdateTimerAndStatusbar);
  connect(ui_.action_monitor_refresh_speed_paused, &QAction::toggled,        this, &Ecalmon::setMonitorUpdatePaused);
  connect(ui_.action_monitor_refresh_now,          &QAction::triggered,      [this]() {updateMonitor(); });

  ui_.action_monitor_refresh_speed_1s->setChecked(true);

  // Log Update Speed selection
  log_update_speed_group_ = new QActionGroup(this);
  log_update_speed_group_->addAction(ui_.action_log_poll_speed_100hz);
  log_update_speed_group_->addAction(ui_.action_log_poll_speed_50hz);
  log_update_speed_group_->addAction(ui_.action_log_poll_speed_20hz);
  log_update_speed_group_->addAction(ui_.action_log_poll_speed_10hz);
  log_update_speed_group_->addAction(ui_.action_log_poll_speed_2hz);
  log_update_speed_group_->addAction(ui_.action_log_poll_speed_1hz);

  connect(log_update_speed_group_,          &QActionGroup::triggered, this,        &Ecalmon::updateLogUpdateTimerAndStatusbar);
  connect(ui_.action_log_poll_speed_paused, &QAction::toggled,        this,        &Ecalmon::setLogUpdatePaused);
  connect(ui_.action_poll_log_now,          &QAction::triggered,      log_widget_, &LogWidget::getEcalLogs);
  connect(log_widget_,                      &LogWidget::paused,       this,        &Ecalmon::setLogUpdatePaused);

  ui_.action_log_poll_speed_20hz->trigger();

  // Log
  connect(ui_.action_clear_log, &QAction::triggered, log_widget_, &LogWidget::clearLog);
  connect(ui_.action_save_log_as, &QAction::triggered, log_widget_, &LogWidget::saveLogAs);

  // Exit
  connect(ui_.action_exit, &QAction::triggered, [this]() {close(); });
  

  // Alternating Row Colors
  connect(ui_.action_alternating_row_colors, &QAction::toggled, 
    [this](bool checked)
  {
    topic_widget_  ->setAlternatingRowColors(checked);
    process_widget_->setAlternatingRowColors(checked);
    host_widget_   ->setAlternatingRowColors(checked);
    service_widget_->setAlternatingRowColors(checked);
  });

  // Parse Time
  connect(ui_.action_show_parsed_times, &QAction::toggled, this, &Ecalmon::setParseTimeEnabled);

#ifdef ECAL_NPCAP_SUPPORT
  connect(ui_.action_npcap_status, &QAction::triggered, this,
      [this]()
      {
        NpcapStatusDialog npcap_status_dialog(this);
        npcap_status_dialog.exec();
      });
#else
  ui_.action_npcap_status->setVisible(false);
#endif // ECAL_NPCAP_SUPPORT

  // Reset layout
  connect(ui_.action_reset_layout, &QAction::triggered, this, &Ecalmon::resetLayout);

  //Plugin settings dialog
  connect(ui_.action_plugin_settings, &QAction::triggered,
    [this]()
    {
      PluginSettingsDialog plugin_settings_dialog(this);
      plugin_settings_dialog.exec();
    });

  // About
  connect(ui_.action_about, &QAction::triggered,
      [this]()
      {
        AboutDialog about_dialog(this);
        about_dialog.exec();
      });

  // License Dialog
  connect(ui_.action_licenses, &QAction::triggered,
      [this]()
      {
        LicenseDialog license_dialog(this);
        license_dialog.exec();
      });

  // Save the initial state for the resetLayout function
  saveInitialState();

  // Load the old window state
  loadGuiSettings();

  // Dock widgets in view menu
  createDockWidgetMenu();

  connect(this, &Ecalmon::monitorUpdatedSignal, [this](eCAL::pb::Monitoring monitoring_pb){topic_widget_->monitorUpdated(monitoring_pb);});

  ui_.action_monitor_refresh_speed_1s->trigger();

  PluginLoader::getInstance()->discover();

  // Restore plugin states
  QSettings settings;
  settings.beginGroup("plugins");
  for (const auto& iid : PluginLoader::getInstance()->getAvailableIIDs())
  {
    if (settings.value(iid, true).toBool())
      PluginLoader::getInstance()->getPluginByIID(iid).load();
  }
  settings.endGroup();
}

Ecalmon::~Ecalmon()
{
  eCAL::Finalize();
}

void Ecalmon::updateMonitor()
{
#ifndef NDEBUG
  qDebug().nospace() << "[" << metaObject()->className() << "] Updating monitor";
#endif // NDEBUG
  std::string monitoring_string;
  eCAL::pb::Monitoring monitoring_pb;

  if (eCAL::Monitoring::GetMonitoring(monitoring_string) && !monitoring_string.empty() && monitoring_pb.ParseFromString(monitoring_string))
  {
    monitor_error_counter_ = 0;
    if (error_label_->isVisible())
    {
      error_label_->setHidden(true);
    }

    emit monitorUpdatedSignal(monitoring_pb);
  }
  else
  {
    monitor_error_counter_++;
    error_label_->setText("  Error getting Monitoring Information [" + QString::number(monitor_error_counter_) + "]  ");
    if (!error_label_->isVisible())
    {
      error_label_->setHidden(false);
    }

#ifndef NDEBUG
    qDebug().nospace() << "[" << metaObject()->className() << "Error getting Monitoring Information";
#endif // NDEBUG
    eCAL::Logging::Log(eCAL_Logging_eLogLevel::log_level_error, "Error getting eCAL Monitoring information");
  }
}


void Ecalmon::setMonitorUpdatePaused(bool paused)
{
  ui_.action_monitor_refresh_speed_paused->blockSignals(true);
  if (ui_.action_monitor_refresh_speed_paused->isChecked() != paused)
  {
    ui_.action_monitor_refresh_speed_paused->setChecked(paused);
  }
  updateMonitorUpdateTimerAndStatusbar();

  ui_.action_monitor_refresh_speed_paused->blockSignals(false);
}

bool Ecalmon::isMonitorUpdatePaused() const
{
  return ui_.action_monitor_refresh_speed_paused->isChecked();
}

void Ecalmon::updateMonitorUpdateTimerAndStatusbar()
{
  if (isMonitorUpdatePaused())
  {
    monitor_update_timer_->stop();
    monitor_update_speed_label_->setText(tr("  Monitor update speed: PAUSED  "));
  }
  else
  {
    QAction* update_speed_action = monitor_update_speed_group_->checkedAction();
    if (update_speed_action)
    {
      if (update_speed_action == ui_.action_monitor_refresh_speed_0_5s)
      {
        monitor_update_timer_->start(500);
        monitor_update_speed_label_->setText(tr("  Monitor update speed: 0.5 s  "));
      }
      else if (update_speed_action == ui_.action_monitor_refresh_speed_1s)
      {
        monitor_update_timer_->start(1000);
        monitor_update_speed_label_->setText(tr("  Monitor update speed: 1 s  "));
      }
      else if (update_speed_action == ui_.action_monitor_refresh_speed_2s)
      {
        monitor_update_timer_->start(2000);
        monitor_update_speed_label_->setText(tr("  Monitor update speed: 2 s  "));
      }
      else if (update_speed_action == ui_.action_monitor_refresh_speed_5s)
      {
        monitor_update_timer_->start(5000);
        monitor_update_speed_label_->setText(tr("  Monitor update speed: 5 s  "));
      }
      else if (update_speed_action == ui_.action_monitor_refresh_speed_10s)
      {
        monitor_update_timer_->start(10000);
        monitor_update_speed_label_->setText(tr("  Monitor update speed: 10 s  "));
      }
      else
      {
        monitor_update_speed_label_->setText(tr("  Monitor update speed: ???  "));
      }
    }
    else
    {
      monitor_update_speed_label_->setText(tr("  Monitor update speed: ???  "));
    }
  }
}


void Ecalmon::setLogUpdatePaused(bool paused)
{
  ui_.action_log_poll_speed_paused->blockSignals(true);
  if (ui_.action_log_poll_speed_paused->isChecked() != paused)
  {
    ui_.action_log_poll_speed_paused->setChecked(paused);
  }
  if (!log_widget_->isPaused())
  {
    log_widget_->blockSignals(true);
    log_widget_->setPaused(paused);
    log_widget_->blockSignals(false);
  }
  updateLogUpdateTimerAndStatusbar();

  ui_.action_log_poll_speed_paused->blockSignals(false);
}

void Ecalmon::updateLogUpdateTimerAndStatusbar()
{
  if (log_widget_->isPaused())
  {
    log_update_speed_label_->setText(tr("  Log frequency: PAUSED  "));
  }
  else
  {
    QAction* update_speed_action = log_update_speed_group_->checkedAction();
    if (update_speed_action)
    {
      if (update_speed_action == ui_.action_log_poll_speed_100hz)
      {
        log_widget_->setPollSpeed(10);
        log_update_speed_label_->setText(tr("  Log frequency: 100 Hz  "));
      }
      else if (update_speed_action == ui_.action_log_poll_speed_50hz)
      {
        log_widget_->setPollSpeed(20);
        log_update_speed_label_->setText(tr("  Log frequency: 50 Hz  "));
      }
      else if (update_speed_action == ui_.action_log_poll_speed_20hz)
      {
        log_widget_->setPollSpeed(50);
        log_update_speed_label_->setText(tr("  Log frequency: 20 Hz  "));
      }
      else if (update_speed_action == ui_.action_log_poll_speed_10hz)
      {
        log_widget_->setPollSpeed(100);
        log_update_speed_label_->setText(tr("  Log frequency: 10 Hz  "));
      }
      else if (update_speed_action == ui_.action_log_poll_speed_2hz)
      {
        log_widget_->setPollSpeed(500);
        log_update_speed_label_->setText(tr("  Log frequency: 2 Hz  "));
      }
      else if (update_speed_action == ui_.action_log_poll_speed_1hz)
      {
        log_widget_->setPollSpeed(1000);
        log_update_speed_label_->setText(tr("  Log frequency: 1 Hz  "));
      }
      else
      {
        log_update_speed_label_->setText(tr("  Log frequency: ??? Hz  "));
      }
    }
    else
    {
      log_update_speed_label_->setText(tr("  Log frequency: ??? Hz  "));
    }
  }
}


bool Ecalmon::isParseTimeEnabled() const
{
  return ui_.action_show_parsed_times->isChecked();
}

void Ecalmon::setParseTimeEnabled(bool enabled)
{
  ui_.action_show_parsed_times->blockSignals(true);
  if (ui_.action_show_parsed_times->isChecked() != enabled)
  {
    ui_.action_show_parsed_times->setChecked(enabled);
  }
  ui_.action_show_parsed_times->blockSignals(false);

  log_widget_  ->setParseTimeEnabled(enabled);
  topic_widget_->setParseTimeEnabled(enabled);
}

void Ecalmon::createDockWidgetMenu()
{
  QList<QDockWidget*> dock_widget_list = findChildren<QDockWidget*>();

  for (QDockWidget* dock_widget : dock_widget_list)
  {
    QAction* view_dock_widget_action = new QAction(dock_widget->windowTitle(), this);
    view_dock_widget_action->setCheckable(true);
    view_dock_widget_action->setChecked(!dock_widget->isHidden());
    view_dock_widget_action->setEnabled(dock_widget->features() & QDockWidget::DockWidgetFeature::DockWidgetClosable);

    ui_.menu_windows->addAction(view_dock_widget_action);

    connect(view_dock_widget_action, &QAction::toggled,
        [dock_widget](bool enabled)
        {
          dock_widget->blockSignals(true);
          if (enabled)
          {
            dock_widget->show();
          }
          else
          {
            dock_widget->close();
          }
          dock_widget->blockSignals(false);
        });

    connect(dock_widget, &QDockWidget::visibilityChanged,
        [view_dock_widget_action, dock_widget]()
        {
          view_dock_widget_action->blockSignals(true);
          view_dock_widget_action->setChecked(dock_widget->isVisible());
          view_dock_widget_action->blockSignals(false);
        });
  }
}


void Ecalmon::closeEvent(QCloseEvent* event)
{
  QSettings settings;
  settings.beginGroup("mainwindow");
  settings.setValue("geometry", saveGeometry());
  settings.setValue("window_state", saveState(EcalmonGlobals::Version()));
  settings.setValue("alternating_row_colors", ui_.action_alternating_row_colors->isChecked());
  settings.setValue("parse_time", isParseTimeEnabled());
  settings.endGroup();

  // save plugin state by iid
  settings.beginGroup("plugins");
  for (const auto& iid : PluginLoader::getInstance()->getAvailableIIDs())
    settings.setValue(iid, PluginLoader::getInstance()->getPluginByIID(iid).isLoaded());
  settings.endGroup();

  event->accept();
}

void Ecalmon::loadGuiSettings()
{
  QSettings settings;
  settings.beginGroup("mainwindow");
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("window_state").toByteArray() , EcalmonGlobals::Version());

  QVariant alternating_row_colors_variant = settings.value("alternating_row_colors");
  if (alternating_row_colors_variant.isValid())
  {
    ui_.action_alternating_row_colors->setChecked(alternating_row_colors_variant.toBool());
  }

  QVariant parse_time_variant = settings.value("parse_time");
  if (parse_time_variant.isValid())
  {
    setParseTimeEnabled(parse_time_variant.toBool());
  }

  settings.endGroup();
}

void Ecalmon::saveInitialState()
{
  initial_geometry_               = saveGeometry();
  initial_state_                  = saveState();
  initial_alternating_row_colors_ = ui_.action_alternating_row_colors->isChecked();
  initial_parse_time_             = isParseTimeEnabled();
}

void Ecalmon::resetLayout()
{
  // Back when we saved the initial window geometry, the window-manager might not have positioned the window on the screen, yet
  int screen_number = QApplication::desktop()->screenNumber(this);

  restoreGeometry(initial_geometry_);
  restoreState(initial_state_);

  ui_.action_alternating_row_colors->setChecked(initial_alternating_row_colors_);

  setParseTimeEnabled(initial_parse_time_);

  move(QApplication::desktop()->availableGeometry(screen_number).center() - rect().center());

  log_widget_    ->resetLayout();
  topic_widget_  ->resetLayout();
  process_widget_->resetLayout();
  host_widget_   ->resetLayout();
  service_widget_->resetLayout();
}

void Ecalmon::updateEcalTime()
{
  auto now = eCAL::Time::ecal_clock::now();
  
  std::string error_message;
  int error_code;
  eCAL::Time::GetStatus(error_code, &error_message);

  QString time_string;

  if (isParseTimeEnabled())
  {
    QDateTime q_ecal_time = QDateTime::fromMSecsSinceEpoch(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()).toUTC();
    time_string = q_ecal_time.toString("yyyy-MM-dd HH:mm:ss.zzz");
  }
  else
  {
    double seconds_since_epoch = std::chrono::duration_cast<std::chrono::duration<double>>(now.time_since_epoch()).count();
    time_string = QString::number(seconds_since_epoch, 'f', 6) + " s";
  }
  time_label_->setText("  eCAL Time: " + time_string + " (Error " + QString::number(error_code) + ": " + error_message.c_str() + ")  ");
}