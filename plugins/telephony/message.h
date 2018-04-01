/**
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

#ifndef PLUGINS_TELEPHONY_MESSAGE_H_
#define PLUGINS_TELEPHONY_MESSAGE_H_

#include <QObject>

class Message: public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.telephony.messages")
    Q_PROPERTY(QString body READ getBody)
    Q_PROPERTY(QString address READ getAddress)
public:
    // TYPE field values from Android
    enum types
    {
        MESSAGE_TYPE_ALL = 0,
        MESSAGE_TYPE_INBOX = 1,
        MESSAGE_TYPE_SENT = 2,
        MESSAGE_TYPE_DRAFT = 3,
        MESSAGE_TYPE_OUTBOX = 4,
        MESSAGE_TYPE_FAILED = 5,
        MESSAGE_TYPE_QUEUED = 6,
    };

    /**
     * Build a new message
     *
     * @param args mapping of field names to values as might be contained in a network packet containing a message
     */
    Message(const QVariantMap& args, QObject* parent = Q_NULLPTR);
    ~Message();

    QString getBody() const { return m_body; }
    QString getAddress() const { return m_address; }

protected:
    /**
     * Body of the message
     */
    QString m_body;

    /**
     * Remote-side address of the message. Most likely a phone number, but may be an email address
     */
    QString m_address;
};

#endif /* PLUGINS_TELEPHONY_MESSAGE_H_ */
