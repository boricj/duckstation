#pragma once
#include "debuggermodels.h"
#include "ui_debuggerwindow.h"
#include <QtWidgets/QMainWindow>
#include <memory>

class DebuggerWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit DebuggerWindow(QWidget* parent = nullptr);
  ~DebuggerWindow();

public Q_SLOTS:
  void onEmulationPaused(bool paused);

  void refreshAll();
  void onCloseActionTriggered();
  void onRunActionTriggered(bool checked);
  void onSingleStepActionTriggered();

private:
  enum class ActiveMemoryRegion
  {
    RAM,
    Scratchpad,
    EXP1,
    Count
  };

  void setupAdditionalUi();
  void connectSignals();
  void disconnectSignals();
  void createModels();
  void setUIEnabled(bool enabled);
  void setActiveMemoryRegion(ActiveMemoryRegion region);

  Ui::DebuggerWindow m_ui;

  std::unique_ptr<DebuggerCodeModel> m_code_model;
  std::unique_ptr<DebuggerRegistersModel> m_registers_model;
  std::unique_ptr<DebuggerStackModel> m_stack_model;

  ActiveMemoryRegion m_active_memory_region = ActiveMemoryRegion::Count;
};
