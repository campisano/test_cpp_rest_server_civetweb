#include <chrono>
#include <iostream>
#include <functional>
#include <map>
#include <memory>
#include <civetweb.h>
#include <string>
#include <thread>

namespace
{
int routingCallback(mg_connection *, void *);
}

using Handler = std::function<int(mg_connection *, void *)>;

class HealthHandler
{
public:
    int handle(mg_connection * _conn, void *)
    {
        std::cout << "health check called" << std::endl;

        std::string body = "{\"status\":\"UP\"}\n";
        unsigned long len = body.length();
        mg_send_http_ok(_conn, "text/plain", len);
        mg_write(_conn, body.c_str(), len);

        return 200; /* HTTP state 200 = OK */
    }
};

class Server
{
public:
    Server()
    {
        m_ctx = NULL;
        mg_init_library(0);
    }

    ~Server()
    {
        stop();
        mg_exit_library();
    }

    void route(
        const std::string & _path,
        const std::string &,
        const Handler & _handler)
    {
        std::cout << _path << std::endl;
        m_handlers.emplace(_path, _handler); // store handler copy
        mg_set_request_handler(m_ctx, _path.c_str(), routingCallback,
                               (void *) & m_handlers[_path]); // use stored handler
    }

    void start(
        const std::string &,
        unsigned int _port,
        unsigned int)
    {
        std::cout << "Service fired up at " << _port << std::endl;

        stop();

        auto port = std::to_string(_port);
        const char * options[] =
        {
            "listening_ports",
            port.c_str(),
            "request_timeout_ms",
            "10000",
            0
        };

        m_ctx = mg_start(NULL, 0, options);

        if(! m_ctx)
        {
            //TODO [CMP] handle error
        }
    }

    void stop()
    {
        if(m_ctx)
        {
            mg_stop(m_ctx);
            m_ctx = NULL;
        }
    }

private:
    mg_context * m_ctx;
    std::map<std::string, const Handler> m_handlers;
};

namespace
{
int routingCallback(mg_connection * _conn, void * _data)
{
    std::cout << "routingCallback" << std::endl;
    const Handler * handler = (Handler *) _data;

    return (*handler)(_conn, NULL);
}
}

int main(const int /*_argc*/, const char ** /*_argv*/)
{
    HealthHandler hh;

    Server server;
    server.start("127.0.0.1", 8080, 2);

    server.route(
        "/health", "NOT_SUPPORTED_METHOD_DEFINITION_AT_THIS_POINT",
        std::bind(&HealthHandler::handle, hh, std::placeholders::_1,
                  std::placeholders::_2));

    // TODO [cmp] handle stop events
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 60 * 60 * 24 * 7));

    return EXIT_SUCCESS;
}
