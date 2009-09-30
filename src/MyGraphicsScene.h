#ifndef MYGRAPHICSSCENE_H
#define MYGRAPHICSSCENE_H

//#include "items/TextContent.h"

#include <QGraphicsScene>
#include <QDataStream>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QTime>



class AbstractContent;
class AbstractVisualItem;
class GenericItemConfig;
class Slide;
class TextItem;

class MyGraphicsScene : public QGraphicsScene
{
	Q_OBJECT
	public:
	//         friend class XmlRead;
	//         friend class XmlSave;
	
		typedef enum ContextHint
                {
                	Editor,
                	Preview,
                	Monitor,
                	Live,
                };
	
		MyGraphicsScene(ContextHint hint = Editor, QObject * parent = 0);
 		~MyGraphicsScene();
 		
 		typedef enum SlideTransition { None, CrossFade };
 		
 		void setSlide(Slide *, SlideTransition t = CrossFade);
 		Slide * slide() { return m_slide; }
		
// 		TextContent * addTextContent();
		
		TextItem * newTextItem(QString text = "Lorem Ipsum");
                AbstractVisualItem * newBoxItem();
                AbstractVisualItem * newVideoItem();
                
                void clear();
                
               
                
                ContextHint contextHint() { return m_contextHint; }
                void setContextHint(ContextHint);
		
	signals:
		void showPropertiesWidget(QWidget * widget);
		
	private:
// 		TextContent * createText(const QPoint & pos);
		void addContent(AbstractContent * content, bool takeOnwership = false); //, const QPoint & pos);
	
		QList<AbstractContent *> m_content;
		QList<AbstractContent *> m_ownedContent;
		QList<GenericItemConfig *> m_configs;
		
		Slide * m_slide;
		Slide * m_slidePrev;
		SlideTransition * m_currentTransition;
		
		ContextHint m_contextHint;
			
	private slots:
		friend class AbstractConfig; // HACK here, only to call 1 method
		friend class GenericItemConfig; // HACK here, only to call 1 method
//         	friend class PixmapButton; // HACK here, only to call 1 method
        
		void slotTransitionStep();
		
		void slotSelectionChanged();
        	void slotConfigureContent(const QPoint & scenePoint);
		void slotStackContent(int);
		void slotDeleteContent();
		void slotDeleteConfig(GenericItemConfig * config);
		void slotApplyLook(quint32 frameClass, bool mirrored, bool allContent);
};

#endif