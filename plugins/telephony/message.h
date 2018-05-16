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
    Q_PROPERTY(QString date READ getDate)
    Q_PROPERTY(QString type READ getType)
    Q_PROPERTY(QString read READ getRead)
    Q_PROPERTY(QString threadID READ getThreadID)

public:
    // TYPE field values from Android
    enum Types
    {
        messageTypeAll = 0,
        messageTypeInbox = 1,
        messageTypeSent = 2,
        messageTypeDraft = 3,
        messageTypeOutbox = 4,
        messageTypeFailed = 5,
        messageTypeQueued = 6,
    };
    Q_ENUM(Types);

    /**
     * Build a new message
     *
     * @param args mapping of field names to values as might be contained in a network packet containing a message
     */
    Message(const QVariantMap& args, QObject* parent = Q_NULLPTR);
    ~Message();

    QString getBody() const { return m_body; }
    QString getAddress() const { return m_address; }
    qint64 getDate() const { return m_date; }
    QString getType() const { return m_type; }
    QString getPerson() const { return m_person; }
    qint32 getRead() const { return m_read; }
    QString getThreadID() const { return m_threadID; }

protected:
    /**
     * Body of the message
     */
    const QString m_body;

    /**
     * Remote-side address of the message. Most likely a phone number, but may be an email address
     */
    const QString m_address;

    /**
     * Date stamp (Unix epoch millis) associated with the message
     */
    const qint64 m_date;

    /**
     * Type of the message. See the message.type enum
     */
    const QString m_type;

    /**
     * Some way of connecting to the contact associated with the message
     */
    const QString m_person;

    /**
     * Whether we have a read report for this message
     */
    const qint32 m_read;

    /**
     * Tag which binds individual messages into a thread
     */
    const QString m_threadID;
};

#endif /* PLUGINS_TELEPHONY_MESSAGE_H_ */
