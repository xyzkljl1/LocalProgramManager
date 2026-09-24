#include "Control.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <Windows.h>

namespace {

const QString ServerName = QStringLiteral("LocalProgramManager.Control.A16999D2");
constexpr int ConnectTimeoutMs = 3000;
constexpr int RequestTimeoutMs = 10 * 60 * 1000;
constexpr int MaximumMessageSize = 64 * 1024;

QByteArray errorResponse(int exitCode, const QString& message)
{
    QJsonObject response;
    response["success"] = false;
    response["exitCode"] = exitCode;
    response["message"] = message;
    return QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n";
}

void writeOutput(const QByteArray& output)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
            return;
        handle = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        return;
    DWORD written = 0;
    WriteFile(handle, output.constData(), static_cast<DWORD>(output.size()), &written, nullptr);
}

}

namespace Control {

bool IsControlMode(int argc, char** argv)
{
    return argc > 1 && QString::fromLocal8Bit(argv[1]).compare("--control", Qt::CaseInsensitive) == 0;
}

int RunClient(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    if (arguments.size() < 4)
    {
        const auto response = errorResponse(InvalidArguments,
            "Usage: LocalProgramManager.exe --control <deploy|restart> <program>");
        writeOutput(response);
        return InvalidArguments;
    }

    const QString command = arguments[2].toLower();
    if (command != "deploy" && command != "restart")
    {
        writeOutput(errorResponse(InvalidArguments, "Unsupported control command."));
        return InvalidArguments;
    }

    QJsonObject request;
    request["version"] = 1;
    request["command"] = command;
    request["program"] = arguments.mid(3).join(" ");
    const QByteArray requestData = QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n";

    QLocalSocket socket;
    socket.connectToServer(ServerName);
    if (!socket.waitForConnected(ConnectTimeoutMs))
    {
        writeOutput(errorResponse(ManagerUnavailable,
            "LocalProgramManager is not running or its control endpoint is unavailable."));
        return ManagerUnavailable;
    }
    if (socket.write(requestData) != requestData.size())
    {
        writeOutput(errorResponse(ManagerUnavailable, "Cannot send the control request."));
        return ManagerUnavailable;
    }
    while (socket.bytesToWrite() > 0)
    {
        if (!socket.waitForBytesWritten(ConnectTimeoutMs))
        {
            writeOutput(errorResponse(ManagerUnavailable, "Cannot send the control request."));
            return ManagerUnavailable;
        }
    }

    QElapsedTimer timer;
    timer.start();
    while (!socket.canReadLine())
    {
        if (socket.bytesAvailable() > MaximumMessageSize)
        {
            writeOutput(errorResponse(ManagerUnavailable, "The manager returned an oversized response."));
            return ManagerUnavailable;
        }
        const int remaining = RequestTimeoutMs - static_cast<int>(timer.elapsed());
        if (remaining <= 0 || !socket.waitForReadyRead(remaining))
        {
            const bool timedOut = timer.elapsed() >= RequestTimeoutMs;
            const int exitCode = timedOut ? ManagerUnresponsive : ManagerUnavailable;
            writeOutput(errorResponse(exitCode, timedOut
                ? "LocalProgramManager did not complete the request within ten minutes."
                : "The manager closed the control connection."));
            return exitCode;
        }
    }

    if (socket.bytesAvailable() > MaximumMessageSize)
    {
        writeOutput(errorResponse(ManagerUnavailable, "The manager returned an oversized response."));
        return ManagerUnavailable;
    }
    QByteArray response = socket.readLine(MaximumMessageSize);
    response.chop(1);
    writeOutput(response + "\n");
    QJsonParseError parseError;
    const QJsonDocument responseDocument = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDocument.isObject())
        return ManagerUnavailable;
    return responseDocument.object()["exitCode"].toInt(ManagerUnavailable);
}

Server::Server()
{
    QObject::connect(&server, &QLocalServer::newConnection, &server,
        [this]() { AcceptConnections(); });
}

bool Server::Start(Handler requestHandler)
{
    if (server.isListening() || !requestHandler)
        return false;
    handler = requestHandler;
    server.setMaxPendingConnections(1);
    server.setSocketOptions(QLocalServer::UserAccessOption);
    if (server.listen(ServerName))
        return true;
    handler = {};
    return false;
}

QString Server::ErrorString() const
{
    return server.errorString();
}

void Server::AcceptConnections()
{
    while (server.hasPendingConnections())
    {
        auto* socket = server.nextPendingConnection();
        socket->setReadBufferSize(MaximumMessageSize + 1);
        QObject::connect(socket, &QLocalSocket::readyRead, socket,
            [this, socket]() { ReadRequest(socket); });
        QObject::connect(socket, &QLocalSocket::disconnected,
            socket, &QLocalSocket::deleteLater);
        if (socket->bytesAvailable() > 0)
            ReadRequest(socket);
    }
}

void Server::ReadRequest(QLocalSocket* socket)
{
    if (socket->property("controlHandled").toBool())
        return;

    if (!socket->canReadLine() && socket->bytesAvailable() <= MaximumMessageSize)
        return;

    socket->setProperty("controlHandled", true);
    QByteArray response;
    if (socket->bytesAvailable() > MaximumMessageSize)
    {
        response = errorResponse(InvalidArguments, "Invalid or oversized control request.");
    }
    else
    {
        QByteArray requestData = socket->readLine(MaximumMessageSize);
        requestData.chop(1);
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(requestData, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            response = errorResponse(InvalidArguments, "Invalid JSON control request.");
        }
        else
        {
            const QJsonObject object = document.object();
            Request request;
            request.command = object["command"].toString().toLower();
            request.program = object["program"].toString();
            response = handler(request);
        }
    }

    if (response.isEmpty())
        response = errorResponse(OperationFailed, "The manager did not produce a response.");
    if (!response.endsWith('\n'))
        response += '\n';
    socket->write(response);
    socket->flush();
    socket->disconnectFromServer();
}

}
