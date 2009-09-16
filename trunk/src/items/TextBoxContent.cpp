#include "TextBoxContent.h"
#include "frames/Frame.h"
#include "items/BezierCubicItem.h"
#include "items/TextProperties.h"
#include "model/TextItem.h"
// #include "CPixmap.h"
#include "RenderOpts.h"
#include <QDebug>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QMimeData>
#include <QPainter>
#include <QTextDocument>
#include <QTextFrame>
#include <QUrl>
#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QTextOption>
#include <QTimer>
#include "items/CornerItem.h"

static QString trimLeft(QString str)
{
	static QRegExp white("\\s");
	while(white.indexIn(str) == 0)
	{
		str = str.right(str.length()-1);
	}
	return str;
}


TextBoxContent::TextBoxContent(QGraphicsScene * scene, QGraphicsItem * parent)
    : AbstractContent(scene, parent, false)
    , m_text(0)
    , m_textRect(0, 0, 0, 0)
    , m_textMargin(4)
    , m_shapeEditor(0)
{
	m_dontSyncToModel = true;
	
	setFrame(0);
	setFrameTextEnabled(false);
	setToolTip(tr("Right click to Edit the text"));
	
	// create a text document
	m_text = new QTextDocument(this);
	
	#if QT_VERSION >= 0x040500
		m_textMargin = (int)m_text->documentMargin();
	#endif
	
	// template text
	QFont font;
	#ifdef Q_OS_WIN
		font.setFamily("Arial");
	#endif
	font.setPointSize(16);
	m_text->setDefaultFont(font);
	m_text->setPlainText(tr("right click to edit..."));
	setHtml(m_text->toHtml());
	
	// shape editor
	m_shapeEditor = new BezierCubicItem(this);
	m_shapeEditor->setVisible(false);
	m_shapeEditor->setControlPoints(QList<QPointF>() << QPointF(-100, -50) << QPointF(-10, 40) << QPointF(100, -50) << QPointF(100, 50));
	connect(m_shapeEditor, SIGNAL(shapeChanged(const QPainterPath &)), this, SLOT(setShapePath(const QPainterPath &)));
	
	m_dontSyncToModel = false;
	
	connect(this, SIGNAL(resized()), this, SLOT(delayContentsResized()));
	
	for(int i=0;i<m_cornerItems.size();i++)
		m_cornerItems.at(i)->setDefaultLeftOp(CornerItem::Scale);
}

TextBoxContent::~TextBoxContent()
{
	delete m_shapeEditor;
	delete m_text;
}

QString TextBoxContent::toHtml() const
{
	return m_text->toHtml();
}

void TextBoxContent::setHtml(const QString & htmlCode)
{
        //qDebug("Setting HTML... [%s]",htmlCode.toAscii().constData());
	m_text->setHtml(htmlCode);
	updateTextConstraints();
        //qDebug("Calling syncToModelItem");
	syncToModelItem(0);
        //qDebug("Done with syncToModelItem()");
}

bool TextBoxContent::hasShape() const
{
	return !m_shapePath.isEmpty();
}

void TextBoxContent::clearShape()
{
	setShapePath(QPainterPath());
	setShapeEditing(false);
	emit notifyHasShape(false);
}

bool TextBoxContent::isShapeEditing() const
{
	return m_shapeEditor->isVisible();
}

void TextBoxContent::setShapeEditing(bool enabled)
{
	if (enabled) 
	{
		// shape editor on
		if (!m_shapeEditor->isVisible()) 
		{
			m_shapeEditor->show();
			emit notifyShapeEditing(true);
		}
	
		// begin new shape
		if (!hasShape()) 
		{
			// use caching only when drawing shaped [disabled because updates are wrong when cached!]
			//setCacheMode(enabled ? QGraphicsItem::DeviceCoordinateCache : QGraphicsItem::NoCache);
		
			// use new shape
			setShapePath(m_shapeEditor->shape());
			emit notifyHasShape(true);
		}
	} 
	else 
	{
		// shape editor off
		if (m_shapeEditor->isVisible()) 
		{
			m_shapeEditor->hide();
			emit notifyShapeEditing(false);
		}
	}
}

QWidget * TextBoxContent::createPropertyWidget()
{
	TextProperties * p = new TextProperties();
	
	// common properties
	connect(p->bFront, SIGNAL(clicked()), this, SLOT(slotStackFront()));
	connect(p->bRaise, SIGNAL(clicked()), this, SLOT(slotStackRaise()));
	connect(p->bLower, SIGNAL(clicked()), this, SLOT(slotStackLower()));
	connect(p->bBack, SIGNAL(clicked()), this, SLOT(slotStackBack()));
	connect(p->bDel, SIGNAL(clicked()), this, SIGNAL(deleteItem()), Qt::QueuedConnection);
	
	// shape properties
	p->bEditShape->setChecked(isShapeEditing());
	connect(this, SIGNAL(notifyShapeEditing(bool)), p->bEditShape, SLOT(setChecked(bool)));
	connect(p->bEditShape, SIGNAL(toggled(bool)), this, SLOT(setShapeEditing(bool)));
	p->bClearShape->setVisible(hasShape());
	connect(this, SIGNAL(notifyHasShape(bool)), p->bClearShape, SLOT(setVisible(bool)));
	connect(p->bClearShape, SIGNAL(clicked()), this, SLOT(clearShape()));
	
	return p;
}

void TextBoxContent::syncFromModelItem(AbstractVisualItem *model)
{
        m_dontSyncToModel = true;
	setModelItem(model);
	
	QFont font;
	TextItem * textModel = dynamic_cast<TextItem*>(model);
	
	if(textModel->text().indexOf('<') < 0)
	{
		m_text->setPlainText(textModel->text());
		textModel->setText(m_text->toHtml());
	}
	setHtml(textModel->text());
	
	font.setFamily(textModel->fontFamily());
	font.setPointSize((int)textModel->fontSize());
	m_text->setDefaultFont(font);
	
	
	bool shapeEnabled = textModel->shapeEnabled();
	if (shapeEnabled)
	{
		QList<QPointF> points;
		QStringList strPoint;
		points << textModel->shapePoint1();
		points << textModel->shapePoint2();
		points << textModel->shapePoint3();
		points << textModel->shapePoint4();
		m_shapeEditor->setControlPoints(points);
	}
	
	AbstractContent::syncFromModelItem(model);
	
        m_dontSyncToModel = false;
}

AbstractVisualItem * TextBoxContent::syncToModelItem(AbstractVisualItem *model)
{
	setModelItemIsChanging(true);
	
	TextItem * textModel = dynamic_cast<TextItem*>(AbstractContent::syncToModelItem(model));
	
	if(!textModel)
	{
		setModelItemIsChanging(false);
                //qDebug("TextBoxContent::syncToModelItem: textModel is null, cannot sync\n");
		return 0;
	}
        //qDebug("TextBoxContent:syncToModelItem: Syncing to model! Yay!");
	textModel->setText(m_text->toHtml());
	textModel->setFontFamily(m_text->defaultFont().family());
	textModel->setFontSize(m_text->defaultFont().pointSize());
	textModel->setShapeEnabled(hasShape());
	if (hasShape()) 
	{
		QList<QPointF> cp = m_shapeEditor->controlPoints();
		textModel->setShapePoint1(cp[0]);
		textModel->setShapePoint2(cp[2]);
		textModel->setShapePoint3(cp[3]);
		textModel->setShapePoint4(cp[4]);
	}
	
	setModelItemIsChanging(false);
	
	return model;
}

QPixmap TextBoxContent::renderContent(const QSize & size, Qt::AspectRatioMode /*ratio*/) const
{
	// get the base empty pixmap
	QSize textSize = boundingRect().size().toSize();
	const float w = size.width(),
			h = size.height(),
			tw = textSize.width(),
			th = textSize.height();
	if (w < 2 || h < 2 || tw < 2 || th < 2)
		return QPixmap();
	
	// draw text (centered, maximized keeping aspect ratio)
	float scale = qMin(w / (tw + 16), h / (th + 16));
	QPixmap pix(size);
	pix.fill(Qt::transparent);
	QPainter pixPainter(&pix);
	pixPainter.translate((w - (int)((float)tw * scale)) / 2, (h - (int)((float)th * scale)) / 2);
	pixPainter.scale(scale, scale);
	m_text->drawContents(&pixPainter);
	pixPainter.end();
	return pix;
}

int TextBoxContent::contentHeightForWidth(int width) const
{
	// if no text size is available, use default
	if (m_textRect.width() < 1 || m_textRect.height() < 1)
		return AbstractContent::contentHeightForWidth(width);
		
	
	int textWidth = width;

	//QRegExp rxWhite("\\s+");
	QRegExp rxWhite("\\b");
	QRegExp rxNonAZ("[A-Za-z0-9\\s]");
	
	// 2. LAYOUT TEXT. find out Block rects and Document rect
	
	QPoint cursor(0,0);
	
	QRect cursorRect(cursor.x(),cursor.y(),0,0);
	
	int blockCount = 0;
	for (QTextBlock tb = m_text->begin(); tb.isValid(); tb = tb.next()) 
	{
		if (!tb.isVisible())
			continue;
		
		QFontMetrics metrics(tb.charFormat().font());
		int textHeight = 1; // wil be set later
					
		QTextBlockFormat tbFormat = tb.blockFormat();
		//int xZero = -(int)tbFormat.leftMargin();
		int xZero = 0;
		cursor.setX(xZero);
		
		int blockTop = cursor.y();
		
		for (QTextBlock::iterator tbIt = tb.begin(); !(tbIt.atEnd()); ++tbIt) 
		{
			QTextFragment frag = tbIt.fragment();
			if (!frag.isValid())
				continue;
		
			QString text = frag.text();
			if (text.trimmed().isEmpty())
				continue;
			QFontMetrics metrics(frag.charFormat().font());
		
			QRect textRect = metrics.boundingRect(text+" ");
			textHeight = textRect.height();
			
			if(cursor.x() + textRect.width() > textWidth)
			{
				QStringList words = text.split(rxWhite);
				// start from end of frag and move backwards
				
				QString tmp;
				QString next;
				for(int i=0;i<words.size();i++)
				{
					tmp += words.at(i);
					
					next = "";
					if(i+1 < words.size())
						next = words.at(i+1);
							
					QRect tmpRect = metrics.boundingRect(tmp+" ");
					if(cursor.x() + tmpRect.width() > textWidth)
					{
						cursor.setX(xZero);
						cursor.setY(cursor.y() + textHeight);
					}
					
					QRect tmpRect2 = metrics.boundingRect(tmp+next+" ");
					if(cursor.x() + tmpRect2.width() >= textWidth)
					{
						// if the next "word" contains a non-A-Z (or 0-9) character (like a period), dont leave it dangling
						if(next != "")
						{
							int pos = rxNonAZ.indexIn(next);
							//qDebug() << "updateTextConstraints(): WrapAlpha: 1: Dangle Check: next:"<<next<<", pos:"<<pos;
							if(pos < 0)
							{
								tmp += next;
								i++;
							}
						}
						
						tmp =  cursor.x() == 0 ? trimLeft(tmp) : tmp;
						if(tmp.trimmed().isEmpty())
							continue;
							
						
						TextLineSpec ts(frag,QRect(cursor - tmpRect.topLeft(), tmpRect.size()), tmp);
						//m_lineSpecs.append(ts);
						
						cursorRect |= ts.rect;
						cursor.setX(xZero);
						cursor.setY(cursor.y() + ts.rect.height());
						
						tmp = "";
					}
				}
				
				if(tmp != "")
				{
					QRect tmpRect = metrics.boundingRect(tmp+" ");
					
					TextLineSpec ts(frag,QRect(cursor - tmpRect.topLeft(), tmpRect.size()),cursor.x() == 0 ? trimLeft(tmp) : tmp);
					//m_lineSpecs.append(ts);
					
					cursorRect |= ts.rect;
					cursor.setX(cursor.x() + ts.rect.width()); //ts.rect.right());
					
						
				}
			}
			else
			{
				TextLineSpec ts(frag,QRect(cursor - textRect.topLeft(), textRect.size()),cursor.x() == 0 ? trimLeft(text) : text);
				//m_lineSpecs.append(ts);
				
				cursorRect |= ts.rect;
				cursor.setX(cursor.x() + ts.rect.width());
			}
		}
		
		
		if(cursor.x() > cursorRect.right())
			cursorRect.setRight(cursor.x());
		if(cursor.y() > cursorRect.bottom())
			cursorRect.setBottom(cursor.y());
			
		cursor.setY(cursor.y() + textHeight);

	}
	
	return cursorRect.height();
}

void TextBoxContent::selectionChanged(bool selected)
{
	// hide shape editing controls
	if (!selected && isShapeEditing())
		setShapeEditing(false);
}

void TextBoxContent::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
	emit backgroundMe();
	QGraphicsItem::mouseDoubleClickEvent(event);
}

void TextBoxContent::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
	// paint parent
	AbstractContent::paint(painter, option, widget);
	
	// check whether we're drawing shaped
	const bool shapedPaint = hasShape() && !m_shapeRect.isEmpty();
	QPointF shapeOffset = m_shapeRect.topLeft();
	
	// scale painter for adapting the Text Rect to the Contents Rect
	QRect cRect = contentsRect();
	QRect sRect = shapedPaint ? m_shapeRect : m_textRect;
	painter->save();
	painter->translate(cRect.topLeft());
	qreal xScale = 1, yScale = 1;
	if (sRect.width() > 0 && sRect.height() > 0)
	{
		xScale = (qreal)cRect.width() / (qreal)sRect.width();
		yScale = (qreal)cRect.height() / (qreal)sRect.height();
		//if (!qFuzzyCompare(xScale, 1.0) || !qFuzzyCompare(yScale, 1.0))
		//painter->scale(xScale, yScale);
		
	}
	
	// shape
	//const bool drawHovering = RenderOpts::HQRendering ? false : isSelected();
	if (shapedPaint)
		painter->translate(-shapeOffset);
	//if (shapedPaint && drawHovering)
	//    painter->strokePath(m_shapePath, QPen(Qt::red, 0));
	
	QPen pen;
	qreal w = 1.5; ///qMax(xScale, yScale);
	//qDebug("Pen Width: %.04f (%.02f, %.02f)",w,xScale,yScale);
 	pen.setWidthF(w);
 	pen.setColor(QColor(0,0,0,255));
 
 	QBrush brush(QColor(255,255,255,255));
	
	
	#if 0
	// standard rich text document drawing
	QAbstractTextDocumentLayout::PaintContext pCtx;
	m_text->documentLayout()->draw(painter, pCtx);
	#else
	#if 0
	
	// manual drawing
	QPointF blockPos = shapedPaint ? QPointF(0, 0) : -m_textRect.topLeft();
	
	//qDebug("paint(): BEGIN");
	//qDebug() << "paint(): Mark1: blockPos="<<blockPos;
	// 1. for each Text Block
	int blockRectIdx = 0;
	for (QTextBlock tb = m_text->begin(); tb.isValid(); tb = tb.next()) 
	{
		if (!tb.isVisible() || blockRectIdx > m_blockRects.size())
			continue;
	
		// 1.1. compute text insertion position
		const QRect & blockRect = m_blockRects[blockRectIdx++];
		QPointF iPos = shapedPaint ? blockPos : blockPos - blockRect.topLeft();
		blockPos += QPointF(0, blockRect.height());
	
		//qDebug() << "paint(): Mark2: blockPos="<<blockPos;
		
		qreal curLen = 8;
	
		// 1.2. iterate over text fragments
		for (QTextBlock::iterator tbIt = tb.begin(); !(tbIt.atEnd()); ++tbIt) 
		{
			QTextFragment frag = tbIt.fragment();
			if (!frag.isValid())
				continue;
		
			// 1.2.1. setup painter and metrics for text fragment
			QTextCharFormat format = frag.charFormat();
			QFont font = format.font();
			painter->setFont(font);
			painter->setPen(format.foreground().color());
			painter->setBrush(Qt::NoBrush);
			
// 			painter->setPen(pen);
// 			painter->setBrush(brush);

			
			QFontMetrics metrics(font);
		
			// 1.2.2. draw each character
			QString text = frag.text();
			foreach (const QChar & textChar, text) 
			{
				if (shapedPaint) 
				{
					// find point on shape and angle
					qreal interval = metrics.width(textChar);
					qreal t = m_shapePath.percentAtLength(curLen);
					QPointF pt = m_shapePath.pointAtPercent(t);
					qreal angle = -m_shapePath.angleAtPercent(t);
			
					// draw rotated letter
					painter->save();
					painter->translate(pt);
					painter->rotate(angle);
					painter->drawText(iPos, textChar);
					painter->restore();
			
					curLen += interval;
				} 
				else 
				{
					painter->drawText(iPos, textChar);
					
					//qDebug() << "paint(): Mark3: iPos="<<iPos;
					
// 					QPainterPath p;
// 					p.addText(iPos,font,textChar);
// 					painter->drawPath(p);
	
					iPos += QPointF(metrics.width(textChar), 0);
				}
			}
		}
	}
	
	#else
	// manual drawing
	//QPointF blockPos = shapedPaint ? QPointF(0, 0) : -m_textRect.topLeft();
	
	//qDebug("paint(): BEGIN");
	//qDebug() << "paint(): Mark1: blockPos="<<blockPos;
	// 1. for each Text Block
	
	for(int i=0; i<m_lineSpecs.size(); i++)
	{
		TextLineSpec ts = m_lineSpecs.at(i);
		
		QTextFragment frag = ts.frag;
		if (!frag.isValid())
			continue;
	
		QTextCharFormat format = frag.charFormat();
		QFont font = format.font();
		painter->setFont(font);
		//painter->setPen(format.foreground().color());
		//painter->setBrush(Qt::NoBrush);
			
		painter->setPen(pen);
		painter->setBrush(brush);
		//painter->setBrush(format.foreground().color());
			
		QFontMetrics metrics(format.font());

		//qDebug() << "paint(): Rect:"<<ts.rect<<", Text:"<<ts.text;
		//painter->drawText(ts.point + QPoint(0,metrics.height()), ts.text);
		//painter->drawText(ts.point, ts.text);
		
		//painter->drawText(ts.rect.topLeft(), ts.text);
// 		QPainterPath p;
// 		p.addText(ts.rect.topLeft(),font,ts.text);
 		painter->drawPath(ts.path);
		
// 		painter->setPen(Qt::red);
// 		painter->setBrush(Qt::NoBrush);
// 		painter->drawRect(ts.rect);
		
		//painter->drawText(ts.rect.topLeft() + QPoint(0,metrics.height()), ts.text);
	}
	
	#endif
	#endif
	
	painter->restore();
}

QPainterPath TextBoxContent::shapePath() const
{
	return m_shapePath;
}

void TextBoxContent::setShapePath(const QPainterPath & path)
{
	if (path == m_shapePath)
		return;
	
	// invalidate rectangles
	m_textRect = QRect();
	m_shapeRect = QRect();
	
	// set new path
	m_shapePath = path;
	
	// regenerate text layouting
	updateTextConstraints();
}


void TextBoxContent::updateTextConstraints()
{
	// 1. actual content stretch
	double prevXScale = 1.0;
	double prevYScale = 1.0;
	if (m_textRect.width() > 0 && m_textRect.height() > 0) {
		QRect cRect = contentsRect();
		prevXScale = (qreal)cRect.width() / (qreal)m_textRect.width();
		prevYScale = (qreal)cRect.height() / (qreal)m_textRect.height();
	}
	
	QTextOption t;
	t.setWrapMode(QTextOption::WordWrap);
	//m_text->documentLayout()->setTextOption(t);
	
	for (QTextBlock tb = m_text->begin(); tb.isValid(); tb = tb.next())
	{
		tb.layout()->setTextOption(t);
	}
	
	m_text->setTextWidth(contentsRect().width());
	
	int textWidth = contentsRect().width();
	
// 	QSizeF sz = m_text->documentLayout()->documentSize();
// 	m_textRect = QRect(QPoint(0,0),QSize((int)sz.width(),(int)sz.height()));
	
	qDebug("updateTextConstraints() BEGIN (width: %d)",textWidth);
	
	//QRegExp rxWhite("\\s+");
	QRegExp rxWhite("\\b");
	QRegExp rxNonAZ("[A-Za-z0-9\\s]");
	
	// 2. LAYOUT TEXT. find out Block rects and Document rect
	int minCharSide = 0;
	m_blockRects.clear();
	m_textRect = QRect(0, 0, 0, 0);
	
	QPoint cursor(0,0);
	m_lineSpecs.clear();
	
	QRect cursorRect(cursor.x(),cursor.y(),0,0);
	
	int blockCount = 0;
	for (QTextBlock tb = m_text->begin(); tb.isValid(); tb = tb.next()) 
	{
		if (!tb.isVisible())
			continue;
		
		//QFontMetrics metrics(tb.charFormat().font());
		//int textHeight = metrics.height();
		int textHeight = 1;
		
					
		QTextBlockFormat tbFormat = tb.blockFormat();
		//int xZero = -(int)tbFormat.leftMargin();
		int xZero = 0;
		cursor.setX(xZero);
		// if this is not the first block, add an empty line between the two
		//cursor.setY(cursor.y() - (int)tbFormat.topMargin() + (blockCount > 0 ? textHeight : 0));//, (int)tbFormat.rightMargin(), (int)tbFormat.bottomMargin());
		//int pre_y = cursor.y();
		//cursor.setY(cursor.y()  + (blockCount > 0 ? textHeight : 0));//, (int)tbFormat.rightMargin(), (int)tbFormat.bottomMargin());
		//qDebug("top-o-the-block: textHeight=%d, y=%d",textHeight,pre_y);
		
		int blockTop = cursor.y();
		
	
		// 2.1.A. calc the Block size uniting Fragments bounding rects
		QRect blockRect(0, 0, 0, 0);
		for (QTextBlock::iterator tbIt = tb.begin(); !(tbIt.atEnd()); ++tbIt) 
		{
			QTextFragment frag = tbIt.fragment();
			if (!frag.isValid())
				continue;
		
			QString text = frag.text();
// 			if (text.trimmed().isEmpty())
// 				continue;
				
			qDebug() << "updateTextConstraints(): 2.1.A.1: frag.text():"<<frag.text();
		
			QFontMetrics metrics(frag.charFormat().font());
			if (!minCharSide || metrics.height() > minCharSide)
				minCharSide = metrics.height();
				
			// TODO: implement superscript / subscript (it's in charFormat's alignment)
			// it must be implemented in paint too
		
			QRect textRect = metrics.boundingRect(text+" ");
			qDebug() << "updateTextConstraints(): 2.1.A.2: metrics.boundingRect(...):"<<textRect;
			
// 			cursor.setY(cursor.y() + textRect.y());
// 			cursor.setX(cursor.x() + textRect.x());

			textHeight = textRect.height();
			
			if(cursor.x() + textRect.width() > textWidth)
			{
				QStringList words = text.split(rxWhite);
				// start from end of frag and move backwards
				
				QString tmp;
				QString next;
				for(int i=0;i<words.size();i++)
				{
					tmp += words.at(i);
					
					
					next = "";
					if(i+1 < words.size())
						next = words.at(i+1);
							
					QRect tmpRect = metrics.boundingRect(tmp+" ");
					if(cursor.x() + tmpRect.width() > textWidth)
					{
						cursor.setX(xZero);
						//cursor.setY(cursor.y() + textRect.height());
						cursor.setY(cursor.y() + textHeight);
						//cursorRect |= ts.rect;
						qDebug() << "updateTextConstraints(): WrapAlpha: 1.2: Text wouldn't fit in current line, wrapping, cursor now at:"<<cursor<<", cursorRect:"<<cursorRect;
					}
					
					
					QRect tmpRect2 = metrics.boundingRect(tmp+next+" ");
					if(cursor.x() + tmpRect2.width() >= textWidth)
					{
						// if the next "word" contains a non-A-Z (or 0-9) character (like a period), dont leave it dangling
						if(next != "")
						{
							int pos = rxNonAZ.indexIn(next);
							//qDebug() << "updateTextConstraints(): WrapAlpha: 1: Dangle Check: next:"<<next<<", pos:"<<pos;
							if(pos < 0)
							{
								tmp += next;
								i++;
							}
						}
						
						tmp =  cursor.x() == 0 ? trimLeft(tmp) : tmp;
						//if(tmp.trimmed().isEmpty())
						//	continue;
							
						
						TextLineSpec ts(frag,QRect(cursor - tmpRect.topLeft(), tmpRect.size()), tmp);
						m_lineSpecs.append(ts);
						
						cursorRect |= ts.rect;
// 						if(cursor.x() + tmpRect.width() > cursorRect.right())
// 							cursorRect.setRight(cursor.x() + tmpRect.width());
// 							
// 						if(cursor.y() + tmpRect.y() < cursorRect.top())
// 							cursorRect.setTop(cursor.y() + tmpRect.y());
							
						cursor.setX(xZero);
						//cursor.setY(cursor.y() + textRect.height());
						cursor.setY(cursor.y() + ts.rect.height());
						
// 						if(cursor.y() + tmpRect.height() > cursorRect.bottom())
// 							cursorRect.setBottom(cursor.y() + tmpRect.height());
						
						qDebug() << "updateTextConstraints(): WrapAlpha: 1: Partial frag used, text:"<<tmp<<", ts.rect:"<<ts.rect<<", cursor now at:"<<cursor<<", cursorRect:"<<cursorRect;
						
						tmp = "";
					}
				}
				
				if(tmp != "")
				{
					QRect tmpRect = metrics.boundingRect(tmp+" ");
					
					TextLineSpec ts(frag,QRect(cursor - tmpRect.topLeft(), tmpRect.size()),cursor.x() == 0 ? trimLeft(tmp) : tmp);
					m_lineSpecs.append(ts);
					
					cursorRect |= ts.rect;
// 					if(cursor.y() + tmpRect.y() < cursorRect.top())
// 						cursorRect.setTop(cursor.y() + tmpRect.y());
							
					cursor.setX(cursor.x() + ts.rect.width()); //ts.rect.right());
					/*
					if(cursor.x() > cursorRect.right())
						cursorRect.setRight(cursor.x());*/
					
					qDebug() << "updateTextConstraints(): WrapAlpha: 1.1: Leftover partial frag, text:"<<tmp<<", ts.rect:"<<ts.rect<<", cursor now at:"<<cursor<<", cursorRect:"<<cursorRect;
						
				}
			}
			else
			{
				TextLineSpec ts(frag,QRect(cursor - textRect.topLeft(), textRect.size()),cursor.x() == 0 ? trimLeft(text) : text);
				m_lineSpecs.append(ts);
				
				cursorRect |= ts.rect;
				cursor.setX(cursor.x() + ts.rect.width()); //ts.rect.right());
// 				if(cursor.y() + textRect.y() < cursorRect.top())
// 					cursorRect.setTop(cursor.y() + textRect.y());
// 				if(cursor.x() > cursorRect.right())
// 					cursorRect.setRight(cursor.x());
				qDebug() << "updateTextConstraints(): WrapAlpha: 2: No break needed, full frag fits. ts.rect:"<<ts.rect<<", cursor now at:"<<cursor<<", cursorRect:"<<cursorRect;
			}
			
// 			if(cursor.x() >= textWidth)
// 			{
// 				if(cursor.x() > cursorRect.right())
// 					cursorRect.setRight(cursor.x());
// 				cursor.setX(xZero);
// 				cursor.setY(cursor.y() + textRect.height());
// 				if(cursor.y() > cursorRect.bottom())
// 					cursorRect.setBottom(cursor.y());
// 				qDebug() << "updateTextConstraints(): 3: WrapAlpha: After adding TextLineSpec, hit end of textWidth, cursor now at:"<<cursor;
// 			}
// 		
			
			if (textRect.left() > 9999)
				continue;
			if (textRect.top() < blockRect.top())
				blockRect.setTop(textRect.top());
			if (textRect.bottom() > blockRect.bottom())
				blockRect.setBottom(textRect.bottom());
		
			//qDebug() << "updateTextConstraints(): 2.1.A: blockRect.1="<<blockRect<<"";
			
			int textWidth = metrics.width(text);
			blockRect.setWidth(blockRect.width() + textWidth);
			
			
			//qDebug() << "updateTextConstraints(): 2.1.A: blockRect.2="<<blockRect<<", textWidth="<<textWidth;
		}
		
		
		// 2.1.B. calc the Block size of blank lines
		if (tb.begin() == tb.end()) 
		{
			blockRect.setWidth(1);
			blockRect.setHeight(textHeight);
			
			//cursor.setX(1);
			//cursor.setY(cursor.y() + textHeight); // + (cursorRect.bottom() - blockTop) + 1);
		
			qDebug() << "updateTextConstraints(): 2.1.B: empty line, blockRect="<<blockRect;
		}
		
		if(cursor.x() > cursorRect.right())
			cursorRect.setRight(cursor.x());
		if(cursor.y() > cursorRect.bottom())
			cursorRect.setBottom(cursor.y());
			
		//if(cursor.x() > 0)
		//{
			//qDebug("** textHeight: %d",textHeight);
			cursor.setY(cursor.y() + textHeight);
		//}
		
		
	
		// 2.2. add the Block's margins
		//QTextBlockFormat tbFormat = tb.blockFormat();
		blockRect.adjust((int)-tbFormat.leftMargin(), (int)-tbFormat.topMargin(), (int)tbFormat.rightMargin(), (int)tbFormat.bottomMargin());
		//cursorRect.adjust((int)-tbFormat.leftMargin(), (int)-tbFormat.topMargin(), (int)tbFormat.rightMargin(), (int)tbFormat.bottomMargin());
		
		//qDebug() << "updateTextConstraints(): 2.2: blockRect="<<blockRect;
	
		// 2.3. store the original block rect
		m_blockRects.append(blockRect);
	
		// 2.4. enlarge the Document rect (uniting the Block rect)
		blockRect.translate(0, m_textRect.bottom() - blockRect.top() + 1);
		
		//qDebug() << "updateTextConstraints(): 2.4: translate by (0,"<<(m_textRect.bottom() - blockRect.top() + 1)<<")";
		
		if (blockRect.left() < m_textRect.left())
			m_textRect.setLeft(blockRect.left());
		if (blockRect.right() > m_textRect.right())
			m_textRect.setRight(blockRect.right());
		if (blockRect.top() < m_textRect.top())
			m_textRect.setTop(blockRect.top());
		if (blockRect.bottom() > m_textRect.bottom())
			m_textRect.setBottom(blockRect.bottom());
			
		qDebug() << "updateTextConstraints(): 2.4: m_textRect="<<m_textRect;
		
		blockCount++;
	}
	m_textRect.adjust(-m_textMargin, -m_textMargin, m_textMargin, m_textMargin);
	
	qDebug() << "updateTextConstraints(): m_textMargin="<<m_textMargin<<" m_textRect="<<m_textRect;
	qDebug() << "updateTextConstraints(): WrapAlpha: cursorRect:"<<cursorRect<<", contentsRect:"<<contentsRect();
	
	m_textRect = cursorRect;
	
	bool changed = false;
	QRect newRect = contentsRect();
	if(m_textRect.height() > newRect.height())
	{
		newRect.setHeight(m_textRect.height());
		changed = true;
	}
// 	if(m_textRect.width() > newRect.width())
// 	{
// 		newRect.setWidth(m_textRect.width());
// 		changed = true;
// 	}
	if(changed)
	{
		resizeContents(newRect);
	}
	
	
// 	QRect un = contentsRect() | m_textRect;
// 	resizeContents(un);
	
	// 3. use shape-based rendering
	if (hasShape()) 
	{
		#if 1
			// more precise, but too close to the path
			m_shapeRect = m_shapePath.boundingRect().toRect();
		#else
			// faster, but less precise (as it uses the controls points to determine
			// the path rect, instead of the path itself)
			m_shapeRect = m_shapePath.controlPointRect().toRect();
		#endif
			minCharSide = qBound(10, minCharSide, 500);
			m_shapeRect.adjust(-minCharSide, -minCharSide, minCharSide, minCharSide);
		
			// FIXME: layout, save layouting and calc the exact size!
			//int w = m_shapeRect.width();
			//int h = m_shapeRect.height();
			//resizeContents(QRect(-w / 2, -h / 2, w, h));
			resizeContents(m_shapeRect);
		
		//      moveBy(m_shapeRect.left(), m_shapeRect.top());
		//        m_shapePath.translate(-m_shapeRect.left(), -m_shapeRect.top());
			//setPos(m_shapeRect.center());
			return;
	}
	
	// 4. resize content keeping stretch
// 	int w = (int)(prevXScale * (qreal)m_textRect.width());
// 	int h = (int)(prevYScale * (qreal)m_textRect.height());
// 	resizeContents(QRect(-w / 2, -h / 2, w, h));
}

void TextBoxContent::delayContentsResized()
{
	QTimer::singleShot(0,this,SLOT(contentsResized()));
}

void TextBoxContent::contentsResized()
{
	//qDebug("contentsResized hit");
 	updateTextConstraints();
 	update();
// 	QRect un = rect | m_textRect;
// 	AbstractContent::resizeContents(un,keepRatio);
//	AbstractContent::resizeContents(rect,keepRatio);
}

void TextBoxContent::updateCache()
{
    /*
    m_cachePixmap = QPixmap(contentsRect().size());
    m_cachePixmap.fill(QColor(0, 0, 0, 0));
    QPainter painter(&m_cachePixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    ...
    */
}
