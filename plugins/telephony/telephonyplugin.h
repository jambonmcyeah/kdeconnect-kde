/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2018 Simon Redman <simon@ergotech.com>
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

#ifndef TELEPHONYPLUGIN_H
#define TELEPHONYPLUGIN_H

#include "message.h"

#include <QLoggingCategory>
#include <QDBusInterface>

#include <core/kdeconnectplugin.h>

/**
 * Packet used to indicate a batch of messages has been pushed from the remote device
 *
 * The body should contain the key "messages" mapping to an array of messages
 *
 * For example:
 * { "messages" : [
 *   { "event" : "sms",
 *     "messageBody" : "Hello",
 *     "phoneNumber" : "2021234567",
 *      "messageDate" : "1518846484880",
 *      "messageType" : "2",
 *      "threadID" : "132"
 *    },
 *    { ... },
 *     ...
 *   ]
 * }
 */
#define PACKET_TYPE_TELEPHONY_MESSAGE QStringLiteral("kdeconnect.telephony.message")

#define PACKET_TYPE_TELEPHONY QStringLiteral("kdeconnect.telephony")

#define PACKET_TYPE_TELEPHONY_REQUEST QStringLiteral("kdeconnect.telephony.request")
#define PACKET_TYPE_SMS_REQUEST QStringLiteral("kdeconnect.sms.request")

/**
 * Packet sent to request all the most-recent messages in all conversations on the device
 *
 * The request packet shall contain no body
 */
#define PACKET_TYPE_TELEPHONY_REQUEST_CONVERSATIONS QStringLiteral("kdeconnect.telephony.request_conversations")

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_TELEPHONY)

class TelephonyPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.telephony")

public:
    explicit TelephonyPlugin(QObject* parent, const QVariantList& args);

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}
    QString dbusPath() const override;

public Q_SLOTS:
    Q_SCRIPTABLE void sendSms(const QString& phoneNumber, const QString& messageBody);

    /**
     * Send a request to the remote for all of its conversations
     */
    Q_SCRIPTABLE void requestAllConversations();

public:
Q_SIGNALS:

private Q_SLOTS:
    void sendMutePacket();
    void showSendSmsDialog();

protected:
    /**
     * Send to the telepathy plugin if it is available
     */
    void forwardToTelepathy(const Message& message);

    /**
     * Handle a packet which contains many messages, such as PACKET_TYPE_TELEPHONY_MESSAGE
     */
    bool handleBatchMessages(const NetworkPacket& np);

private:
    QDBusInterface m_telepathyInterface;
};

#endif
