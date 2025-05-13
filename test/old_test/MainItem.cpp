#include "MainItem.h"
#include <QPainter>


CMainItem::CMainItem(QGraphicsItem *parent /*= Q_NULLPTR*/) : QGraphicsPixmapItem(parent)
{
	m_rcItem = QRect(0, 0, 0, 0);	
// 	setFlag(QGraphicsItem::ItemClipsToShape);
}

void CMainItem::setRect(QRect v_rcItem)
{
	m_rcItem.setRect(0, 0, v_rcItem.width(), v_rcItem.height());
	prepareGeometryChange();
	update();
}

void CMainItem::setRect(QRectF v_rcItem)
{
	m_rcItem.setRect(0, 0, v_rcItem.width(), v_rcItem.height());
	prepareGeometryChange();
	update();
}

QPainterPath CMainItem::shape() const
{
	QPainterPath path;
	path.addEllipse(m_rcItem);
	return path;
}

QRectF CMainItem::boundingRect() const
{
	return m_rcItem;
}

int CMainItem::type() const
{
	return Type;
}

void CMainItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget /* = Q_NULLPTR */)
{
// 	painter->setRenderHint(QPainter::SmoothPixmapTransform);
    return QGraphicsPixmapItem::paint(painter, option, widget);
}
