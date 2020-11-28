#include "debuggermodels.h"
#include "common/log.h"
#include "core/cpu_core.h"
#include "core/cpu_core_private.h"
#include "core/cpu_disasm.h"
#include <QtGui/QColor>
#include <QtGui/QPalette>
#include <QtWidgets/QApplication>
Log_SetChannel(DebuggerModels);

static constexpr int NUM_COLUMNS = 3;
static constexpr int STACK_RANGE = 128;
static constexpr u32 STACK_VALUE_SIZE = sizeof(u32);

DebuggerCodeModel::DebuggerCodeModel(QObject* parent /*= nullptr*/) : QAbstractTableModel(parent)
{
  resetCodeView(0);
}

DebuggerCodeModel::~DebuggerCodeModel() {}

int DebuggerCodeModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return static_cast<int>((m_code_region_end - m_code_region_start) / CPU::INSTRUCTION_SIZE);
}

int DebuggerCodeModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return NUM_COLUMNS;
}

int DebuggerCodeModel::getRowForAddress(VirtualMemoryAddress address) const
{
  return static_cast<int>((address - m_code_region_start) / CPU::INSTRUCTION_SIZE);
}

VirtualMemoryAddress DebuggerCodeModel::getAddressForRow(int row) const
{
  return m_code_region_start + (static_cast<u32>(row) * CPU::INSTRUCTION_SIZE);
}

int DebuggerCodeModel::getRowForPC() const
{
  return getRowForAddress(m_last_pc);
}

QVariant DebuggerCodeModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
  if (index.column() < 0 || index.column() >= NUM_COLUMNS)
    return QVariant();

  if (role == Qt::DisplayRole)
  {
    const VirtualMemoryAddress address = getAddressForRow(index.row());
    switch (index.column())
    {
      case 0:
      {
        // Address
        return QVariant(QString::asprintf("0x%08X", address));
      }

      case 1:
      {
        // Bytes
        u32 instruction_bits;
        if (!CPU::SafeReadInstruction(address, &instruction_bits))
          return QStringLiteral("<invalid>");

        return QString::asprintf("%08X", instruction_bits);
      }

      case 2:
      {
        // Instruction
        u32 instruction_bits;
        if (!CPU::SafeReadInstruction(address, &instruction_bits))
          return QStringLiteral("<invalid>");

        Log_DevPrintf("Disassemble %08X", address);
        SmallString str;
        CPU::DisassembleInstruction(&str, address, instruction_bits, nullptr);
        return QString::fromUtf8(str.GetCharArray(), static_cast<int>(str.GetLength()));
      }

      default:
        return QVariant();
    }
  }
  else if (role == Qt::BackgroundRole)
  {
    const VirtualMemoryAddress address = getAddressForRow(index.row());

    // breakpoint
    // return QVariant(QColor(171, 97, 107));
//     if (address == m_last_pc)
//       return QApplication::palette().toolTipBase();
    if (address == m_last_pc)
      return QColor(100, 100, 0);
    else
      return QVariant();
  }
  else if (role == Qt::ForegroundRole)
  {
    const VirtualMemoryAddress address = getAddressForRow(index.row());

    // breakpoint
    // return QVariant(QColor(171, 97, 107));
//     if (address == m_last_pc)
//       return QApplication::palette().toolTipText();
    if (address == m_last_pc)
      return QColor(Qt::white);
    else
      return QVariant();
  }
  else
  {
    return QVariant();
  }
}

QVariant DebuggerCodeModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (orientation != Qt::Horizontal)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  static const char* header_names[] = {"Address", "Bytes", "Instruction"};
  if (section < 0 || section >= countof(header_names))
    return QVariant();

  return header_names[section];
}

bool DebuggerCodeModel::updateRegion(VirtualMemoryAddress address)
{
  CPU::Segment segment = CPU::GetSegmentForAddress(address);
  std::optional<Bus::CodeRegion> region = Bus::GetCodeRegionForAddress(CPU::VirtualAddressToPhysical(address));
  if (!region.has_value() || (segment == m_current_segment && region == m_current_code_region))
    return false;

  const VirtualMemoryAddress start_address =
    CPU::PhysicalAddressToVirtual(Bus::GetCodeRegionStart(region.value()), segment);
  const VirtualMemoryAddress end_address =
    CPU::PhysicalAddressToVirtual(Bus::GetCodeRegionEnd(region.value()), segment);

  beginResetModel();
  m_code_region_start = start_address;
  m_code_region_end = end_address;
  endResetModel();
  return true;
}

void DebuggerCodeModel::resetCodeView(VirtualMemoryAddress start_address)
{
  updateRegion(start_address);
}

void DebuggerCodeModel::setPC(VirtualMemoryAddress pc)
{
  const VirtualMemoryAddress prev_pc = m_last_pc;

  m_last_pc = pc;
  if (!updateRegion(pc))
  {
    const int old_row = getRowForAddress(prev_pc);
    emit dataChanged(index(old_row, 0), index(old_row, NUM_COLUMNS - 1));

    const int new_row = getRowForAddress(pc);
    emit dataChanged(index(new_row, 0), index(new_row, NUM_COLUMNS - 1));
  }
}

DebuggerRegistersModel::DebuggerRegistersModel(QObject* parent /*= nullptr*/) : QAbstractListModel(parent) {}

DebuggerRegistersModel::~DebuggerRegistersModel() {}

int DebuggerRegistersModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return static_cast<int>(CPU::Reg::count);
}

int DebuggerRegistersModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant DebuggerRegistersModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
  u32 reg_index = static_cast<u32>(index.row());
  if (reg_index > static_cast<u32>(CPU::Reg::count))
    return QVariant();

  if (index.column() < 0 || index.column() > 1)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  if (index.column() == 0)
  {
    return QString::fromUtf8(CPU::GetRegName(static_cast<CPU::Reg>(reg_index)));
  }
  else
  {
    return QString::asprintf("0x%08X", CPU::g_state.regs.r[reg_index]);
  }
}

QVariant DebuggerRegistersModel::headerData(int section, Qt::Orientation orientation,
                                            int role /*= Qt::DisplayRole*/) const
{
  if (orientation != Qt::Horizontal)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  static const char* header_names[] = {"Register", "Value"};
  if (section < 0 || section >= countof(header_names))
    return QVariant();

  return header_names[section];
}

void DebuggerRegistersModel::invalidateView()
{
  beginResetModel();
  endResetModel();
}

DebuggerStackModel::DebuggerStackModel(QObject* parent /*= nullptr*/) : QAbstractListModel(parent) {}

DebuggerStackModel::~DebuggerStackModel() {}

int DebuggerStackModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return STACK_RANGE * 2;
}

int DebuggerStackModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant DebuggerStackModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
  if (index.column() < 0 || index.column() > 1)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  const u32 sp = CPU::g_state.regs.sp;
  const VirtualMemoryAddress address =
    (sp - static_cast<u32>(STACK_RANGE * STACK_VALUE_SIZE)) + static_cast<u32>(index.row()) * STACK_VALUE_SIZE;

  if (index.column() == 0)
    return QString::asprintf("0x%08X", address);

  u32 value;
  if (!CPU::SafeReadMemoryWord(address, &value))
    return QStringLiteral("<invalid>");

  return QString::asprintf("0x%08X", ZeroExtend32(value));
}

QVariant DebuggerStackModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (orientation != Qt::Horizontal)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  static const char* header_names[] = {"Address", "Value"};
  if (section < 0 || section >= countof(header_names))
    return QVariant();

  return header_names[section];
}

void DebuggerStackModel::invalidateView()
{
  beginResetModel();
  endResetModel();
}
