#include <iostream>

#include <boost/asynchronous/extensions/http/http_server.hpp>

#include <boost/asynchronous/extensions/asio/asio_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>

namespace http = boost::asynchronous::http;

std::string html_encode(const std::string& data)
{
    // Simply encode every character as a character entity. This is overkill, but works for sure.
    std::ostringstream result;
    for (const char c : data)
        result << "&#" << static_cast<int>(static_cast<unsigned char>(c)) << ";";
    return result.str();
}

void route_main(std::shared_ptr<http::connection_proxy> conn, http::request&& req, std::function<void(http::response&&)> cb)
{
    boost::regex query("[?&]q=([^&]+)");
    boost::regex type("[?&]t=([^&]+)");
    boost::regex status("[?&]s=(\\d+)");
    boost::smatch what;
    
    std::string message = "Hello there!";
    std::string mime = "text/html";
    int http_code = 200;

    if (boost::regex_search(req.uri, what, query))
        message = what[1].str();
    if (boost::regex_search(req.uri, what, type))
        mime = what[1].str();
    if (boost::regex_search(req.uri, what, status))
    {
        // If there is a status code given, it must be valid; otherwise, return 400 Bad Request.
        try { http_code = std::stoi(what[1].str()); } catch (...) { http_code = 400; return; }
        if (http::categorize(static_cast<http::status>(http_code)) == http::status_category::unknown)
            http_code = 400;   
    }
    
    if (http_code != 200)
    {
        cb(req.make_response(static_cast<http::status>(http_code)));
        return;
    }

    std::string response = "<!DOCTYPE html><html><head><title>Asynchronous HTTP</title></head><body style=\"font-family: monospace;\"><pre>" + html_encode(message) + "</pre><hr /><p style=\"font-size: x-small\">Hosted with <a href=\"/about\"><code>Asynchronous</code></a> - Try sending <a href=\"/form\"><code>POST</code> requests</a></p></body></html>\n";    
    cb(req.make_response(http::status::ok, http::content { response, mime }));
}

void route_about(std::shared_ptr<http::connection_proxy> conn, http::request&& req, std::function<void(http::response&&)> cb)
{
    cb(req.make_response(http::status::temporary_redirect, http::content {}, {{ "Location", "https://github.com/henry-ch/asynchronous" }}));
}

void route_form_get(std::shared_ptr<http::connection_proxy> conn, http::request&& req, std::function<void(http::response&&)> cb)
{
    std::string response ="<!DOCTYPE html><html><head><title>Asynchronous HTTP</title></head><body style=\"font-family: monospace;\"><form action=\"/form\" method=\"post\"><label for=\"data\"><code>Data:</code></label><input style=\"margin-left: 20px;\" type=\"text\" id=\"data\" name=\"data\" placeholder=\"Your data\" /><button style=\"margin-left: 20px\" type=\"submit\">Submit</button></form><hr /><p style=\"font-size: x-small\">Hosted with <a href=\"/about\"><code>Asynchronous</code></a> - <a href=\"/\">Home</a></p></body></html>\n";
    cb(req.make_response(http::status::ok, http::content { response, "text/html" }));
}

void route_form_post(std::shared_ptr<http::connection_proxy> conn, http::request&& req, std::function<void(http::response&&)> cb)
{
    std::string response = "<!DOCTYPE html><html><head><title>Asynchronous HTTP</title></head><body style=\"font-family: monospace;\">Your request body is: <code style=\"margin-left: 20px\">" + html_encode(req.body) + "</code><hr /><p style=\"font-size: x-small\">Hosted with <a href=\"/about\"><code>Asynchronous</code></a> - <a href=\"/\">Home</a></p></body></html>\n";
    cb(req.make_response(http::status::ok, http::content { response, "text/html" }));
}

namespace ctrl_c
{
    // Handle cancellation
    std::atomic_flag cancelled = ATOMIC_FLAG_INIT; // We need this to ensure we don't set_value twice, that would crash us.
    std::shared_ptr<std::promise<void>> cancellation_promise;

    void handle_signal(int /* signo */)
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        if (!cancelled.test_and_set())
        cancellation_promise->set_value();
    }
    void wait()
    {
        cancellation_promise = std::make_shared<std::promise<void>>();
        auto future = cancellation_promise->get_future();
        signal(SIGINT, handle_signal);
        signal(SIGTERM, handle_signal);
        future.get();
    }
}

int main(int argc, char *argv[])
{
    BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(
        if (argc != 3 && argc != 5)
        {
            std::cerr << "Usage: " << argv[0] << " <host> <port> [<certificate chain> <private key>]\n";
            std::cerr << "You can generate a certificate via\n    openssl req -x509 -nodes -newkey rsa:2048 -subj \"/C=<country code>/ST=<state or province>/L=<location>/O=<organization>/OU=<department>/CN=<domain name>\" -keyout key.pem -out cert.pem -days 90\n";
            std::cerr << "If you do not specify any certificate arguments, the server will respond to plain HTTP requests and not use SSL/TLS.\n";
            return 1;
        }
    )
    BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK(
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
            std::cerr << "SSL/TLS is disabled.\n";
            std::cerr << "To enable HTTPS support, undefine BOOST_ASYNCHRONOUS_HTTP_NO_SSL.\n";
            return 1;
        }
    )
    int index = 1;
    std::string host = argv[index++];
    unsigned port = std::atoi(argv[index++]);
    
    bool https = argc > 3;
    std::string certfile, keyfile;
    if (https)
    {
        certfile = argv[index++];
        keyfile = argv[index++];
    }

    // Set up threading
    auto max_threads = boost::thread::hardware_concurrency();
    auto server_sched = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::single_thread_scheduler<boost::asynchronous::lockfree_queue<BOOST_ASYNCHRONOUS_HTTP_JOB_TYPE>>());
    auto asio_sched = boost::asynchronous::create_shared_scheduler_proxy(new boost::asynchronous::asio_scheduler<>(max_threads));

    // Set up HTTPS
    http::security sec;
    if (https)
        sec.https = http::security::https_config { certfile, keyfile };
    else
        sec.https = boost::none;

    // Start the server
    http::server_proxy server(server_sched, std::move(asio_sched), host, port, sec);
    server.add_route({ "GET" }, "/", route_main);
    server.add_route({ "GET" }, "/about", route_about);
    server.add_route({ "GET" }, "/form", route_form_get);
    server.add_route({ "POST" }, "/form", route_form_post);
    
    // Wait.
    ctrl_c::wait();
}
