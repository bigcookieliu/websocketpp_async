#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <vector>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <memory>
#include <cstdlib>

struct WebSocketServer
{
    using server = websocketpp::server<websocketpp::config::asio>;
    using message_ptr = server::message_ptr;

    void Listen(int port)
    {
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;
        using websocketpp::lib::bind;

        mServer.init_asio();
        mServer.set_message_handler(bind(&WebSocketServer::OnMessage, this, _1, _2));
        mServer.set_close_handler(bind(&WebSocketServer::OnClose, this, _1));

        websocketpp::lib::error_code err;
        mServer.listen(websocketpp::lib::asio::ip::tcp::v4(), port, err);

        if (err)
            throw std::runtime_error(err.message());

        mServer.start_accept();
        mServer.poll();
    }

    void Send()
    {
        if (m_connections.empty())
            return;

        auto& hdl = m_connections.back();

        mServer.send(hdl, "{\"fx_underlyings\": [\"FOO\", \"BAR\"], \"otc_underlyings\": []}", websocketpp::frame::opcode::text);
        mServer.close(hdl, 0, "bye");
    }

    void Poll()
    {
        auto sz = mServer.poll();

        if (sz > 0)
            std::cout << sz << " handlers processed" << std::endl;
    }

    void OnMessage(websocketpp::connection_hdl hdl, message_ptr msg)
    {
        std::cout << "on_message called with hdl: " << hdl.lock().get() << " and message: " << msg->get_payload() << std::endl;
        m_connections.emplace_back(hdl);
    }

    void OnClose(websocketpp::connection_hdl hdl)
    {
        std::cout << "on_close called with hdl: " << hdl.lock().get() << std::endl;

        auto it = std::find_if(m_connections.begin(), m_connections.end(), [&](const auto& lhs) { return lhs.lock().get() == hdl.lock().get(); });
        assert(it != m_connections.end());
        m_connections.erase(it);
    }

private:
    server mServer;

    std::vector<websocketpp::connection_hdl> m_connections;
};

int main(int argc, char** argv)
{
    if (argc != 2)
        throw std::runtime_error("usage: " + std::string(argv[0]) + " <port>");

    int port = std::atoi(argv[1]);

    WebSocketServer server;
    server.Listen(port);

    // simulating a busy event loop that polls
    while (1)
    {
        server.Poll();
        server.Send();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
