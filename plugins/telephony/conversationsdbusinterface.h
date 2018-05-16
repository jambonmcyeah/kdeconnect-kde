/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONVERSATIONSDBUSINTERFACE_H
#define CONVERSATIONSDBUSINTERFACE_H

#include <QDBusAbstractAdaptor>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QPointer>

#include "interfaces/dbusinterfaces.h"
#include "message.h"

class KdeConnectPlugin;
class Device;

class ConversationsDbusInterface
    : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.conversations")

public:
    explicit ConversationsDbusInterface(KdeConnectPlugin* plugin);
    ~ConversationsDbusInterface() override;

    void addMessage(Message* message);
    void removeMessage(const QString& internalID);

public Q_SLOTS:
    /**
     * Return a list of all currently-valid threadIDs
     */
    QStringList activeConversations();

    /**
     * Send a new message to this conversation
     */
    void replyToConversation(const QString& conversationID, const QString& message);

Q_SIGNALS:
    Q_SCRIPTABLE void messagePosted(const QString& publicId);
    Q_SCRIPTABLE void messageRemoved(const QString& publicId);
    Q_SCRIPTABLE void messageUpdated(const QString& publicId);

private /*methods*/:
    void removeNotification(const QString& internalId);
    QString newId(); //Generates successive identifitiers to use as public ids
    void notificationReady();

private /*attributes*/:
    const Device* m_device;
    KdeConnectPlugin* m_plugin;
    QHash<QString, QList<QPointer<Message>>> m_conversations;
    QHash<QString, QString> m_internalIdToPublicId;
    int m_lastId;

    TelephonyDbusInterface m_telephonyInterface;
};

#endif // CONVERSATIONSDBUSINTERFACE_H
