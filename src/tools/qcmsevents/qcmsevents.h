/** qcmsevents.h
 *
 *  A small X11 color management event observer.
 *
 *  License: newBSD
 *  Copyright: (c)2009-2015 - Kai-Uwe Behrmann <ku.b@gmx.de>
 *
 */

#ifndef QCMSEVENTS_H
#define QCMSEVENTS_H

#include <QtGui>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QDialog>
#include <QDesktopWidget>
#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#if QT_VERSION >= 0x050000
#include <QtX11Extras/QX11Info>
#else
#include <QtGui/QX11Info>
#endif
#include <cstdlib>
#include <cstring>

#include <X11/Xcm/XcmEvents.h>
#include <X11/Xcm/Xcm.h>
#include "oyranos_sentinel.h"

class QcmseDialog : public QDialog
{
  Q_OBJECT

  public:
    QcmseDialog();

  protected:
    void close( QCloseEvent * e );

  public slots:
    void quit() { oyFinish_( 0 ); exit(0); }
    void setIcon(int index);
    void showConfig();
    void showNM();

  public:
    QSystemTrayIcon * icon;
    void createIcon();
  private:
    QMenu * systrayIconMenu;
    QAction * quitA;
    QAction * showA;
    QAction * showC;
    QAction * showN;

    QListWidget * log_list;
    QComboBox * icons;
    int index_;
  public:
    void log( const char * text, int code );
    int init;
    int index() { return index_; }
private slots:
    void onAction(QAction*a);
};

extern QcmseDialog * dialog;



#if QT_VERSION >= 0x050000
class Qcmse : public QApplication, QAbstractNativeEventFilter
{
  XcmeContext_s * c;
  public:
    Qcmse(int & argc, char ** argv) : QApplication(argc,argv)
    {
      c = NULL;
      QCoreApplication::eventDispatcher()->installNativeEventFilter( this );
    };
    ~Qcmse()
    {
      XcmeContext_Release( &c );
    };
    void setup()
    {
      const char * display_name = getenv("DISPLAY");
      if(QX11Info::isPlatformX11())
        c = XcmeContext_Create( display_name );
    };
    // ask Qt for new arriving events
    virtual bool nativeEventFilter(const QByteArray &, void *, long *) Q_DECL_OVERRIDE
    {
      // use Xlib for Xcm's stand alone X11 context 
      while(XPending(XcmeContext_DisplayGet( c )))
      {
        XEvent event;
        XNextEvent( XcmeContext_DisplayGet( c ), &event);
        XcmeContext_InLoop( c, &event );
      }

      return false;
    };
};
#else /* QT_VERSION >= 0x050000 */
class Qcmse : public QApplication
{
  XcmeContext_s * c;
  public:
    Qcmse(int & argc, char ** argv) : QApplication(argc,argv)
    {
      c = XcmeContext_New( );
    };
    ~Qcmse()
    {
      XcmeContext_Release( &c );
    };
    void setup()
    {
      const char * display_name = getenv("DISPLAY");
      QDesktopWidget * d = this->desktop();
      QX11Info i = d->x11Info();
      XcmeContext_DisplaySet( c, i.display() );
      XcmeContext_WindowSet( c, dialog->winId() );
      XcmeContext_Setup( c, display_name );
    };
    bool x11EventFilter( XEvent * event )
    {
      /* set the actual X11 Display, Qt seems to change the old pointer. */
      XcmeContext_DisplaySet( c, event->xany.display );
      /* process the X event */
      XcmeContext_InLoop( c, event );
      return false; 
    };
};
#endif /* QT_VERSION >= 0x050000 */

#endif /* QCMSEVENTS_H */

