/*
*	MainItem
*/
#ifndef __MAINITEM_H__
#define __MAINITEM_H__
#include <QGraphicsPixmapItem>
#include <QRectF>


class CMainItem : public QGraphicsPixmapItem
{
// 	Q_OBJECT

public:
	explicit CMainItem(QGraphicsItem *parent = Q_NULLPTR);

	void setRect(QRect v_rcItem);
	void setRect(QRectF v_rcItem);

	QPainterPath shape() const override;
	QRectF boundingRect() const override;
	enum { Type = UserType + 7 };
	int type() const Q_DECL_OVERRIDE;

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget /* = Q_NULLPTR */) override;

private:
	QRect m_rcItem;// Item区域

};

#endif // !__MAINITEM_H__
