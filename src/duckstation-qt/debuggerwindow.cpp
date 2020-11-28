#include "debuggerwindow.h"
#include "core/cpu_core_private.h"
#include "qthostinterface.h"
#include <QtWidgets/QFileDialog>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QMessageBox>

DebuggerWindow::DebuggerWindow(QWidget* parent /* = nullptr */) : QMainWindow(parent)
{
  m_ui.setupUi(this);
  setupAdditionalUi();
  connectSignals();
  createModels();
  setActiveMemoryRegion(ActiveMemoryRegion::RAM);
  setUIEnabled(false);
}

DebuggerWindow::~DebuggerWindow() {}

void DebuggerWindow::onEmulationPaused(bool paused)
{
  if (paused)
  {
    setUIEnabled(true);
    refreshAll();
  }
  else
  {
    setUIEnabled(false);
  }
}

void DebuggerWindow::refreshAll()
{
  m_registers_model->invalidateView();
  m_stack_model->invalidateView();
  m_ui.memoryView->repaint();

  m_code_model->setPC(CPU::g_state.regs.pc);
  int row = m_code_model->getRowForPC();
  if (row >= 0)
  {
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    m_ui.codeView->scrollTo(m_code_model->index(row, 0));
  }
}

void DebuggerWindow::onCloseActionTriggered() {}

void DebuggerWindow::onRunActionTriggered(bool checked)
{
  //   if (m_debugger_interface->IsStepping() == !checked)
  //     return;
  //
  //   m_debugger_interface->SetStepping(checked);
}

void DebuggerWindow::onSingleStepActionTriggered()
{
  /*  m_debugger_interface->SetStepping(true, 1);*/
}

void DebuggerWindow::setupAdditionalUi()
{
#ifdef WIN32
  QFont fixedFont;
  fixedFont.setFamily(QStringLiteral("Consolas"));
  fixedFont.setFixedPitch(true);
  fixedFont.setStyleHint(QFont::TypeWriter);
  fixedFont.setPointSize(10);
#else
  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#endif
  m_ui.codeView->setFont(fixedFont);
  m_ui.registerView->setFont(fixedFont);
  m_ui.memoryView->setFont(fixedFont);
  m_ui.stackView->setFont(fixedFont);

  setCentralWidget(nullptr);
  delete m_ui.centralwidget;
}

void DebuggerWindow::connectSignals()
{
  QtHostInterface* hi = QtHostInterface::GetInstance();
  connect(hi, &QtHostInterface::emulationPaused, this, &DebuggerWindow::onEmulationPaused);

  connect(m_ui.actionPause_Continue, SIGNAL(triggered(bool)), this, SLOT(onRunActionTriggered(bool)));
  connect(m_ui.actionStep_Into, SIGNAL(triggered()), this, SLOT(onSingleStepActionTriggered()));
  connect(m_ui.action_Close, SIGNAL(triggered()), this, SLOT(onCloseActionTriggered()));
}

void DebuggerWindow::disconnectSignals()
{
  QtHostInterface* hi = QtHostInterface::GetInstance();
  hi->disconnect(this);
}

void DebuggerWindow::createModels()
{
  m_code_model = std::make_unique<DebuggerCodeModel>();
  m_ui.codeView->setModel(m_code_model.get());

  m_registers_model = std::make_unique<DebuggerRegistersModel>();
  m_ui.registerView->setModel(m_registers_model.get());
  // m_ui->registerView->resizeRowsToContents();

  m_stack_model = std::make_unique<DebuggerStackModel>();
  m_ui.stackView->setModel(m_stack_model.get());
}

void DebuggerWindow::setUIEnabled(bool enabled)
{
  // Disable all UI elements that depend on execution state
  m_ui.actionPause_Continue->setChecked(!enabled);
  m_ui.codeView->setDisabled(!enabled);
  m_ui.registerView->setDisabled(!enabled);
  m_ui.stackView->setDisabled(!enabled);
  // m_ui->tabMemoryView
}

void DebuggerWindow::setActiveMemoryRegion(ActiveMemoryRegion region)
{
  switch (region)
  {
    case ActiveMemoryRegion::RAM:
      m_ui.memoryView->setData(Bus::RAM_BASE, Bus::g_ram, Bus::RAM_SIZE);
      break;

    case ActiveMemoryRegion::Scratchpad:
      m_ui.memoryView->setData(CPU::DCACHE_LOCATION, CPU::g_state.dcache.data(), CPU::DCACHE_SIZE);
      break;

    case ActiveMemoryRegion::EXP1:
    default:
      m_ui.memoryView->setData(Bus::EXP1_BASE, nullptr, 0);
      break;
  }

  m_ui.memoryView->repaint();
}
