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

#include "conversationsdbusinterface.h"
#include "message.h"

#include <QDBusConnection>

#include <core/device.h>
#include <core/kdeconnectplugin.h>

#include "telephonyplugin.h"
#include "sendreplydialog.h"

ConversationsDbusInterface::ConversationsDbusInterface(KdeConnectPlugin* plugin)
    : QDBusAbstractAdaptor(const_cast<Device*>(plugin->device()))
    , m_device(plugin->device())
    , m_plugin(plugin)
    , m_lastId(0)
{

}

ConversationsDbusInterface::~ConversationsDbusInterface()
{
    qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Destroying ConversationsDbusInterface";
}

QStringList ConversationsDbusInterface::activeConversations()
{
    return m_conversations.keys();
}

void ConversationsDbusInterface::addMessage(Message* message)
{
    const QString& internalId = newId();

    if (m_internalIdToPublicId.contains(internalId)) {
        removeMessage(internalId);
    }

    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "addNotification" << internalId;

    const QString& publicId = newId();
    m_conversations[QString::number(message->getThreadID())].append(message);
    m_internalIdToPublicId[internalId] = publicId;

    QDBusConnection::sessionBus().registerObject(m_device->dbusPath()+"/messages/"+publicId, message, QDBusConnection::ExportScriptableContents);
    Q_EMIT messagePosted(publicId);
}

void ConversationsDbusInterface::removeMessage(const QString& internalID)
{
    // TODO: Delete the specified message from our internal structures
}

void ConversationsDbusInterface::replyToConversation(const QString& conversationID, const QString& message)
{
    // TODO: Use DBus to call sendSMS of the device's telephony plugin. In the future, support more cool things.
}

QString ConversationsDbusInterface::newId()
{
    return QString::number(++m_lastId);
}
