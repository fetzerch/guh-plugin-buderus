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

#include "devicepluginbuderus.h"
#include "plugininfo.h"

#include "Qt-AES/qaesencryption.h"

#include <QJsonDocument>

namespace {
const char *uuidUrl = "/gateway/uuid";
const char *versionFirmwareUrl = "/gateway/versionFirmware";
const char *temperatureOutdoorUrl = "/system/sensors/temperatures/outdoor_t1";
}

DeviceManager::HardwareResources DevicePluginBuderus::requiredHardware() const
{
    return DeviceManager::HardwareResourceNetworkManager | DeviceManager::HardwareResourceTimer;
}

DeviceManager::DeviceSetupStatus DevicePluginBuderus::setupDevice(Device *device)
{
    if (device->deviceClassId() != buderusGatewayDeviceClassId) {
        return DeviceManager::DeviceSetupStatusFailure;
    }

    qCDebug(dcBuderus) << "Configuring device" << device->paramValue(buderusHostParamTypeId).toString();
    sendAsyncRequest(Request{uuidUrl, buderusIdParamTypeId, device});
    sendAsyncRequest(Request{versionFirmwareUrl, buderusVersionParamTypeId, device});
    return DeviceManager::DeviceSetupStatusAsync;
}

void DevicePluginBuderus::guhTimer()
{
    foreach (Device *device, myDevices()) {
        sendAsyncRequest(Request{temperatureOutdoorUrl, temperatureOutdoorStateTypeId, device});
    }
}

void DevicePluginBuderus::sendAsyncRequest(const Request &request)
{
    auto host = request.device->paramValue(buderusHostParamTypeId).toString();
    QNetworkRequest networkRequest(QUrl(QString("http://%1%2").arg(host).arg(request.url)));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, "TeleHeater/2.2.3");
    networkRequest.setRawHeader("Accept", "application/json");
    qCDebug(dcBuderus) << "Sending request:" << request.url;
    auto reply = networkManagerGet(networkRequest);
    m_asyncRequests.insert(reply, request);
}

void DevicePluginBuderus::networkManagerReplyReady(QNetworkReply *reply)
{
    if (!m_asyncRequests.contains(reply))
        return;

    auto request = m_asyncRequests.take(reply);
    if (!reply->error()) {
        auto value = parseValue(request.device, reply->readAll());
        qCDebug(dcBuderus) << "Received value:" << request.url << value;
        if (request.type == Request::Type::Param) {
            request.device->setParamValue(request.param, value);
            if (request.url == versionFirmwareUrl) {
                emit deviceSetupFinished(request.device, DeviceManager::DeviceSetupStatusSuccess);
            }
        } else if (request.type == Request::Type::State) {
            request.device->setStateValue(request.state, value);
        }

        // Update the timestamp only for 1 value per request type.
        if (request.url == temperatureOutdoorUrl || request.url == versionFirmwareUrl) {
            request.device->setStateValue(updateTimeStateTypeId, QDateTime::currentSecsSinceEpoch());
        }
    } else {
        qCWarning(dcBuderus) << "Reply error" << reply->errorString();
        if (request.url == versionFirmwareUrl) {
            emit deviceSetupFinished(request.device, DeviceManager::DeviceSetupStatusFailure);
        }
    }
    reply->deleteLater();
}

void DevicePluginBuderus::deviceRemoved(Device *device)
{
    // Remove pending requests.
    for (auto it = m_asyncRequests.begin(); it != m_asyncRequests.end();) {
        if (it.value().device->id() == device->id()) {
            it.key()->deleteLater();
            it = m_asyncRequests.erase(it);
        } else {
            ++it;
        }
    }
}

QVariant DevicePluginBuderus::parseValue(Device *device, const QByteArray &responseText,
                                         const QString &key)
{
    QByteArray decryptionKey = device->paramValue(buderusKeyParamTypeId).toString().toLocal8Bit();
    auto text = decrypt(responseText, decryptionKey);
    auto json = QJsonDocument::fromJson(text);
    auto result = json.object().value(key).toVariant();
    return result;
}

QByteArray DevicePluginBuderus::decrypt(const QByteArray &encrypted, const QByteArray &key)
{
    QAESEncryption encryption{QAESEncryption::AES_256, QAESEncryption::ECB};
    auto base64decoded = QByteArray::fromBase64(encrypted);
    auto decrypted = encryption.decode(base64decoded, QByteArray::fromHex(key));
    while (decrypted.endsWith('\0')) decrypted.chop(1);
    return decrypted;
}
