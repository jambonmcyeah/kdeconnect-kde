/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "conversationlistmodel.h"

#include <QLoggingCategory>
#include "interfaces/conversationmessage.h"

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL, "kdeconnect.sms.conversations_list")

ConversationListModel::ConversationListModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    qCCritical(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Constructing" << this;
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    setItemRoleNames(roles);

    ConversationMessage::registerDbusType();
}

ConversationListModel::~ConversationListModel()
{
    if (m_conversationsInterface) delete m_conversationsInterface;
}

void ConversationListModel::setDeviceId(const QString& deviceId)
{
    qCCritical(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "setDeviceId" << "of" << this;
    if (m_conversationsInterface) delete m_conversationsInterface;

    m_deviceId = deviceId;

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId);

    connect(m_conversationsInterface, SIGNAL(conversationCreated(const QString&)),
            this, SLOT(handleCreatedConversation(const QString&)), Qt::UniqueConnection);

    prepareConversationsList();

    m_conversationsInterface->requestAllConversationThreads();
}

void ConversationListModel::prepareConversationsList()
{
    clear();

    QDBusPendingReply<QStringList> validThreadIDsReply = m_conversationsInterface->activeConversations();
    validThreadIDsReply.waitForFinished();

    for (const QString& conversationId : validThreadIDsReply.value())
    {
        handleCreatedConversation(conversationId);
    }

}

void ConversationListModel::handleCreatedConversation(const QString& conversationId)
{
    QVariantList args;
    args.append(conversationId);

    m_conversationsInterface->callWithCallback("getFirstFromConversation", args,
                                               this, SLOT(createRowFromMessage(ConversationMessage)), SLOT(printDBusError(QDBusError)));
}

void ConversationListModel::printDBusError(const QDBusError& error)
{
    qCWarning(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << error;
}

void ConversationListModel::createRowFromMessage(const ConversationMessage& message)
{
    appendRow(new QStandardItem(message.getBody()));
}
