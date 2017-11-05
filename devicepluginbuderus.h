/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Christian Fetzer <fetzer.ch@gmail.com>              *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DEVICEPLUGINBUDERUS_H
#define DEVICEPLUGINBUDERUS_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"

class DevicePluginBuderus: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "guru.guh.DevicePlugin" FILE "devicepluginbuderus.json")
    Q_INTERFACES(DevicePlugin)

public:
    DeviceManager::HardwareResources requiredHardware() const override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;

    void guhTimer() override;
    void networkManagerReplyReady(QNetworkReply *reply) override;

    void deviceRemoved(Device *device) override;

private:
    struct Request {
        enum class Type {
            Unknown,
            Param,
            State
        };

        Request(const char *url, const ParamTypeId &paramTypeId, Device *device)
            : url{url}, param{paramTypeId}, device{device}, type{Type::Param}
        {}

        Request(const char *url, const StateTypeId &stateTypeId, Device *device)
            : url{url}, state{stateTypeId}, device{device}, type{Type::State}
        {}

        Request() = default;

        QString url;
        ParamTypeId param;
        StateTypeId state;
        Device *device = nullptr;
        Type type = Type::Unknown;
    };

    void sendAsyncRequest(const Request &request);
    QVariant parseValue(Device *device, const QByteArray &responseText,
                        const QString &key = QStringLiteral("value"));
    static QByteArray decrypt(const QByteArray &encrypted, const QByteArray &key);

private:
    QMap<QNetworkReply*, Request> m_asyncRequests;
};

#endif // DEVICEPLUGINBUDERUS_H
