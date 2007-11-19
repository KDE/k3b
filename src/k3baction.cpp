#include <k3baction.h>

KAction* K3b::createAction( QObject* parent,
                           const QString& text, const QString& icon, const
                           QKeySequence& shortcut, QObject* receiver, const char* slot,
                           KActionCollection* actionCollection,
                           const QString& actionName )
{
        KAction* action = new KAction( parent );
        action->setText( text );
        if( !icon.isEmpty() ) {
            action->setIcon( KIcon( icon ) );
        }
        action->setShortcut( shortcut );
        if( receiver ) {
            QObject::connect( action, SIGNAL( triggered() ),
                              receiver, slot );
        }
        if( actionCollection ) {
            actionCollection->addAction( actionName, action );
        }
        return action;
}

