#include "memoryviewwidget.h"
#include <QtGui/QPainter>
#include <QtWidgets/QScrollBar>

MemoryViewWidget::MemoryViewWidget(QWidget* parent /* = nullptr */, size_t address_offset /* = 0 */,
                                   const void* data_ptr /* = nullptr */, size_t data_size /* = 0 */)
  : QAbstractScrollArea(parent)
{
  nBlockAddress = 2;
  mBytesPerLine = 16;

  updateMetrics();

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &MemoryViewWidget::adjustContent);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &MemoryViewWidget::adjustContent);

  if (data_ptr)
    setData(address_offset, data_ptr, data_size);
}

MemoryViewWidget::~MemoryViewWidget() = default;

int MemoryViewWidget::addressWidth()
{
  return (nBlockAddress * 4 + nBlockAddress - 1) * pxWidth;
}

int MemoryViewWidget::hexWidth()
{
  return (mBytesPerLine * 4 + 1) * pxWidth;
}

int MemoryViewWidget::asciiWidth()
{
  return (mBytesPerLine * 2 + 1) * pxWidth;
}

void MemoryViewWidget::updateMetrics()
{
  pxWidth = fontMetrics().horizontalAdvance(QChar('0'));
  pxHeight = fontMetrics().height();
}

void MemoryViewWidget::setData(size_t address_offset, const void* data_ptr, size_t data_size)
{
  m_data = data_ptr;
  m_data_size = data_size;
  m_address_offset = address_offset;
  adjustContent();
}

void MemoryViewWidget::setFont(const QFont& font)
{
  QAbstractScrollArea::setFont(font);
  updateMetrics();
}

void MemoryViewWidget::resizeEvent(QResizeEvent*)
{
  adjustContent();
}

void MemoryViewWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(viewport());
  painter.setFont(font());
  if (!m_data)
    return;

  int offsetX = horizontalScrollBar()->value();

  int y = pxHeight;
  QString address;

  painter.setPen(viewport()->palette().color(QPalette::WindowText));

  const unsigned num_rows = static_cast<unsigned>(endPos - startPos) / mBytesPerLine;
  for (unsigned row = 0; row <= num_rows; row++)
  {
    const size_t address = m_address_offset + startPos + static_cast<unsigned>(row * mBytesPerLine);
    const QString address_text(QString::asprintf("%08X", address));
    painter.drawText(pxWidth / 2 - offsetX, y, address_text);
    y += pxHeight;
  }

  int x;
  int lx = addressWidth() + pxWidth;
  painter.drawLine(lx - offsetX, 0, lx - offsetX, height());
  lx += pxWidth / 2;
  y = pxHeight;

  // hex data
  x = lx - offsetX + 4 * pxWidth;
  int w = 3 * pxWidth;
  for (int col = 0; col < mBytesPerLine / 2; col++)
  {
    painter.fillRect(x - pxWidth / 4, 0, w, height(), viewport()->palette().color(QPalette::AlternateBase));
    x += 8 * pxWidth;
  }

  size_t offset = startPos;
  for (unsigned row = 0; row <= num_rows; row++)
  {
    x = lx - offsetX;
    for (unsigned col = 0; col < mBytesPerLine && offset < m_data_size; col++, offset++)
    {
      unsigned char value;
      std::memcpy(&value, static_cast<const unsigned char*>(m_data) + offset, sizeof(value));
      const QString hex_text(QString::asprintf("%02X", value));
      painter.drawText(x, y, hex_text);
      x += 4 * pxWidth;
    }
    y += pxHeight;
  }

  lx = addressWidth() + hexWidth();
  painter.drawLine(lx - offsetX, 0, lx - offsetX, height());

  lx += pxWidth / 2;

  y = pxHeight;
  offset = startPos;
  for (unsigned row = 0; row <= num_rows; row++)
  {
    x = lx - offsetX;
    for (unsigned col = 0; col < mBytesPerLine && offset < m_data_size; col++, offset++)
    {
      unsigned char value;
      std::memcpy(&value, static_cast<const unsigned char*>(m_data) + offset, sizeof(value));
      if (value < 0x20)
        value = '.';
      painter.drawText(x, y, static_cast<QChar>(value));
      x += 2 * pxWidth;
    }
    y += pxHeight;
  }
}

void MemoryViewWidget::adjustContent()
{
  if (!m_data)
  {
    setEnabled(false);
    return;
  }

  setEnabled(true);

  int w = addressWidth() + hexWidth() + asciiWidth();
  horizontalScrollBar()->setRange(0, w - viewport()->width());
  horizontalScrollBar()->setPageStep(viewport()->width());

  nRowsVisible = viewport()->height() / pxHeight;
  int val = verticalScrollBar()->value();
  startPos = (size_t)val * mBytesPerLine;
  endPos = startPos + nRowsVisible * mBytesPerLine - 1;
  if (endPos >= m_data_size)
    endPos = m_data_size - 1;

  const int lineCount = static_cast<int>(m_data_size / mBytesPerLine);
  verticalScrollBar()->setRange(0, lineCount - nRowsVisible);
  verticalScrollBar()->setPageStep(nRowsVisible);

  viewport()->update();
}