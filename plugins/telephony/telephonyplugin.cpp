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

#include "telephonyplugin.h"

#include "sendreplydialog.h"

#include <KLocalizedString>
#include <QDebug>
#include <QDBusReply>

#include <KPluginFactory>
#include "message.h"

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_telephony.json", registerPlugin< TelephonyPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_TELEPHONY, "kdeconnect.plugin.telephony")

TelephonyPlugin::TelephonyPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_telepathyInterface(QStringLiteral("org.freedesktop.Telepathy.ConnectionManager.kdeconnect"), QStringLiteral("/kdeconnect"))
{
}

bool TelephonyPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("isCancel"))) {

        //TODO: Clear the old notification
        return true;
    }

    const QString& event = np.get<QString>(QStringLiteral("event"), QStringLiteral("unknown"));

    // Handle old-style packets
    if (np.type() == PACKET_TYPE_TELEPHONY && event == QLatin1String("sms"))
    {
        Message message(np.body());
        this->forwardToTelepathy(message);
    }

    if (np.type() == PACKET_TYPE_TELEPHONY_MESSAGE)
    {
        return this->handleBatchMessages(np);
    }

    return true;
}

void TelephonyPlugin::sendMutePacket()
{
    NetworkPacket packet(PACKET_TYPE_TELEPHONY_REQUEST, {{"action", "mute"}});
    sendPacket(packet);
}

void TelephonyPlugin::sendSms(const QString& phoneNumber, const QString& messageBody)
{
    NetworkPacket np(PACKET_TYPE_SMS_REQUEST, {
        {"sendSms", true},
        {"phoneNumber", phoneNumber},
        {"messageBody", messageBody}
    });
    qDebug() << "sending sms!";
    sendPacket(np);
}

void TelephonyPlugin::showSendSmsDialog()
{
    QString phoneNumber = sender()->property("phoneNumber").toString();
    QString contactName = sender()->property("contactName").toString();
    QString originalMessage = sender()->property("originalMessage").toString();
    SendReplyDialog* dialog = new SendReplyDialog(originalMessage, phoneNumber, contactName);
    connect(dialog, &SendReplyDialog::sendReply, this, &TelephonyPlugin::sendSms);
    dialog->show();
    dialog->raise();
}

void TelephonyPlugin::sendAllConversationsRequest()
{
    NetworkPacket np(PACKET_TYPE_TELEPHONY_REQUEST_CONVERSATIONS);

    sendPacket(np);
}

bool TelephonyPlugin::forwardToTelepathy(const Message& message)
{
    // In case telepathy can handle the message, don't do anything else
    if (m_telepathyInterface.isValid()) {
        qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Passing a text message to the telepathy interface";
        connect(&m_telepathyInterface, SIGNAL(messageReceived(QString,QString)), SLOT(sendSms(QString,QString)), Qt::UniqueConnection);
        const QString messageBody = message.getBody();
        const QString contactName = "";
        const QString phoneNumber = message.getAddress();
        QDBusReply<bool> reply = m_telepathyInterface.call(QStringLiteral("sendMessage"), phoneNumber, contactName, messageBody);
        if (reply) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

bool TelephonyPlugin::handleBatchMessages(const NetworkPacket& np)
{
    auto messages = np.get<QVariantList>("messages");

    for (QVariant body : messages)
    {
        Message message(body.toMap());
        this->forwardToTelepathy(message);
    }

    return true;
}

QString TelephonyPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/telephony";
}

#include "telephonyplugin.moc"
