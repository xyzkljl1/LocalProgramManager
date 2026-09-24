#pragma once

#include <QByteArray>
#include <QLocalServer>
#include <QString>
#include <functional>

class QLocalSocket;

namespace Control {

enum ExitCode {
    Success = 0,
    InvalidArguments = 1,
    ManagerUnavailable = 2,
    ProgramNotFound = 3,
    OperationFailed = 4,
    ManagerUnresponsive = 5,
    AlreadyDeploying = 6
};

struct Request {
    QString command;
    QString program;
};

using Handler = std::function<QByteArray(const Request&)>;

bool IsControlMode(int argc, char** argv);
int RunClient(int argc, char** argv);

class Server {
public:
    Server();

    bool Start(Handler handler);
    QString ErrorString() const;

private:
    void AcceptConnections();
    void ReadRequest(QLocalSocket* socket);

    QLocalServer server;
    Handler handler;
};

}
