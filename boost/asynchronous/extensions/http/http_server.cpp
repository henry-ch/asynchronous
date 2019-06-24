#include "http_server.hpp"

#include <algorithm>
#include <clocale>
#include <iostream>
#include <limits>
#include <sstream>

// ==========================================================================
// Configuration

#ifndef BOOST_ASYNCHRONOUS_HTTP_MAX_HEADER_LENGTH
// Maximum length of the entire HTTP header, sans body.
# define BOOST_ASYNCHRONOUS_HTTP_MAX_HEADER_LENGTH (1ULL << 16)
#endif
#ifndef BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_LENGTH
// Maximum default size of a single request chunk is 64 MiB. If you would like larger requests, I really do recommend splitting them up into multiple requests or using Transfer-Encoding: Chunked instead of increasing this value - this helps protect a bit against resource exhaustion from one big inbound request.
# define BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_LENGTH (1ULL << 26)
#endif
#ifndef BOOST_ASYNCHRONOUS_HTTP_DEFAULT_CONTENT_TYPE
// The default content MIME type of a response if none is given
# define BOOST_ASYNCHRONOUS_HTTP_DEFAULT_CONTENT_TYPE "text/plain"
#endif
#ifndef BOOST_ASYNCHRONOUS_HTTP_BODYLESS_METHODS
// Known request methods that do not (necessarily) send a body, so we do not attempt to read one.
# define BOOST_ASYNCHRONOUS_HTTP_BODYLESS_METHODS std::set<std::string> { "GET", "HEAD", "DELETE", "TRACE", "OPTIONS", "CONNECT" }
#endif

#define BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_HEADER_LENGTH (std::numeric_limits<std::size_t>::digits10 + 3) // + 3 because we need + 1 (digits10 guarantees _any_ number of that size fits into the type), plus \r\n.

// ==========================================================================
// Logging

// To enable logging, define BOOST_ASYNCHRONOUS_HTTP_LOG_LEVEL to the minimum boost::asynchronous::http::log_severity you want to log.
// You can define custom logging endpoints by defining the BOOST_ASYNCHRONOUS_HTTP_LOG_ENDPOINT macro to any function taking a boost::asynchronous::http::log_severity and a std::string.
// Default logging uses terminal colors based on the severity of the message. If you define a custom endpoint, you can enable this behavior by explicitly defining BOOST_ASYNCHRONOUS_HTTP_LOG_FORCE_COLORS.
// Messages come with a newline at the end - no need to add your own.
#ifndef BOOST_ASYNCHRONOUS_HTTP_LOG_LEVEL
# define BOOST_ASYNCHRONOUS_HTTP_LOG_INFO(...) do {} while (0)
# define BOOST_ASYNCHRONOUS_HTTP_LOG_ERROR(...) do {} while (0)
# define BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(status, endpoint, ...) do {} while (0)
#else
# define BOOST_ASYNCHRONOUS_HTTP_LOG_INFO(...) do { ::log_message(boost::asynchronous::http::log_severity::info) << __VA_ARGS__; } while (0)
# define BOOST_ASYNCHRONOUS_HTTP_LOG_ERROR(...) do { ::log_message(boost::asynchronous::http::log_severity::error) << __VA_ARGS__; } while (0)
# define BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(status, endpoint, ...) do { std::ostringstream endpoint_stream; endpoint_stream << endpoint; BOOST_ASYNCHRONOUS_HTTP_LOG_INFO(std::left << std::setw(35) << status << "  " << std::setw(21) << endpoint_stream.str() << "  " << __VA_ARGS__); } while (0)
#endif
#ifndef BOOST_ASYNCHRONOUS_HTTP_LOG_ENDPOINT
# define BOOST_ASYNCHRONOUS_HTTP_LOG_ENDPOINT ::log_message::default_dispatch
# ifndef BOOST_ASYNCHRONOUS_HTTP_LOG_FORCE_COLORS
#  define BOOST_ASYNCHRONOUS_HTTP_LOG_FORCE_COLORS
# endif
#endif

namespace
{

class log_message : public std::ostringstream
{
public:
    using severity_cmp_t = std::underlying_type<boost::asynchronous::http::log_severity>::type;

    log_message(boost::asynchronous::http::log_severity severity)
        : m_severity(severity)
    {
#ifdef BOOST_ASYNCHRONOUS_HTTP_LOG_FORCE_COLORS
        switch (m_severity)
        {
            case boost::asynchronous::http::log_severity::info:
                // No coloring for normal output.
                break;
            case boost::asynchronous::http::log_severity::error:
                *this << "\x1b[31;1m";
                break;
            default:
                *this << "\x1b[33;1m"; // Shouldn't usually happen, but if it does, yellow sounds good.
                break;
        }
#endif
    }
    
    virtual ~log_message()
    {
#ifdef BOOST_ASYNCHRONOUS_HTTP_LOG_FORCE_COLORS
        *this << "\x1b[0m\n";
#else
        *this << "\n";
#endif
        if (static_cast<severity_cmp_t>(m_severity) >= static_cast<severity_cmp_t>(BOOST_ASYNCHRONOUS_HTTP_LOG_LEVEL))
        {
            BOOST_ASYNCHRONOUS_HTTP_LOG_ENDPOINT(m_severity, str());
        }
    }
    
    static void default_dispatch(boost::asynchronous::http::log_severity /* severity */, const std::string& str)
    {
        std::clog << str << std::flush;
    }

private:
    boost::asynchronous::http::log_severity m_severity;
};

}

// ==========================================================================
// Utility functions

namespace
{

std::size_t move_streambuf_contents(boost::asio::streambuf& target, boost::asio::streambuf& source)
{
    std::size_t bytes = boost::asio::buffer_copy(target.prepare(source.size()), source.data());
    target.commit(bytes);
    return bytes;
}

}

namespace boost { namespace asynchronous { namespace http
{

// ==========================================================================
// Miscellaneous

std::ostream& operator<<(std::ostream& stream, boost::asynchronous::http::status code)
{
    switch (code)
    {
        case boost::asynchronous::http::status::continue_: stream << "100 Continue"; break;
        case boost::asynchronous::http::status::switching_protocols: stream << "101 Switching Protocols"; break;
        case boost::asynchronous::http::status::processing: stream << "102 Processing"; break;
        case boost::asynchronous::http::status::early_hints: stream << "103 Early Hints"; break;

        case boost::asynchronous::http::status::ok: stream << "200 OK"; break;
        case boost::asynchronous::http::status::created: stream << "201 Created"; break;
        case boost::asynchronous::http::status::accepted: stream << "202 Accepted"; break;
        case boost::asynchronous::http::status::non_authoritative_information: stream << "203 Non-Authoritative Information"; break;
        case boost::asynchronous::http::status::no_content: stream << "204 No Content"; break;
        case boost::asynchronous::http::status::reset_content: stream << "205 Reset Content"; break;
        case boost::asynchronous::http::status::partial_content: stream << "206 Partial Content"; break;

        case boost::asynchronous::http::status::multiple_choices: stream << "300 Multiple Choices"; break;
        case boost::asynchronous::http::status::moved_permanently: stream << "301 Moved Permanently"; break;
        case boost::asynchronous::http::status::found: stream << "302 Found"; break;
        case boost::asynchronous::http::status::see_other: stream << "303 See Other"; break;
        case boost::asynchronous::http::status::not_modified: stream << "304 Not Modified"; break;
        case boost::asynchronous::http::status::use_proxy: stream << "305 Use Proxy"; break;
        case boost::asynchronous::http::status::switch_proxy: stream << "306 Switch Proxy"; break;
        case boost::asynchronous::http::status::temporary_redirect: stream << "307 Temporary Redirect"; break;
        case boost::asynchronous::http::status::permanent_redirect: stream << "308 Permanent Redirect"; break;

        case boost::asynchronous::http::status::bad_request: stream << "400 Bad Request"; break;
        case boost::asynchronous::http::status::unauthorized: stream << "401 Unauthorized"; break;
        case boost::asynchronous::http::status::payment_required: stream << "402 Payment Required"; break;
        case boost::asynchronous::http::status::forbidden: stream << "403 Forbidden"; break;
        case boost::asynchronous::http::status::not_found: stream << "404 Not Found"; break;
        case boost::asynchronous::http::status::method_not_allowed: stream << "405 Method Not Allowed"; break;
        case boost::asynchronous::http::status::not_acceptable: stream << "406 Not Acceptable"; break;
        case boost::asynchronous::http::status::proxy_authentication_required: stream << "407 Proxy Authentication Required"; break;
        case boost::asynchronous::http::status::request_timeout: stream << "408 Request Timeout"; break;
        case boost::asynchronous::http::status::conflict: stream << "409 Conflict"; break;
        case boost::asynchronous::http::status::gone: stream << "410 Gone"; break;
        case boost::asynchronous::http::status::length_required: stream << "411 Length Required"; break;
        case boost::asynchronous::http::status::precondition_failed: stream << "412 Precondition Failed"; break;
        case boost::asynchronous::http::status::payload_too_large: stream << "413 Payload Too Large"; break;
        case boost::asynchronous::http::status::uri_too_long: stream << "414 URI Too Long"; break;
        case boost::asynchronous::http::status::unsupported_media_type: stream << "415 Unsupported Media Type"; break;
        case boost::asynchronous::http::status::range_not_satisfiable: stream << "416 Range Not Satisfiable"; break;
        case boost::asynchronous::http::status::expectation_failed: stream << "417 Expectation Failed"; break;
        case boost::asynchronous::http::status::misdirected_request: stream << "421 Misdirected Rrequest"; break;
        case boost::asynchronous::http::status::too_early: stream << "425 Too Early"; break;
        case boost::asynchronous::http::status::upgrade_required: stream << "426 Upgrade Required"; break;
        case boost::asynchronous::http::status::precondition_required: stream << "428 Precondition Required"; break;
        case boost::asynchronous::http::status::too_many_requests: stream << "429 Too Many Requests"; break;
        case boost::asynchronous::http::status::request_header_fields_too_large: stream << "431 Request Header Fields Too Large"; break;
        case boost::asynchronous::http::status::unavailable_for_legal_reasons: stream << "451 Unavailable For Legal Reasons"; break;

        case boost::asynchronous::http::status::internal_server_error: stream << "500 Internal Server Error"; break;
        case boost::asynchronous::http::status::not_implemented: stream << "501 Not Implemented"; break;
        case boost::asynchronous::http::status::bad_gateway: stream << "502 Bad Gateway"; break;
        case boost::asynchronous::http::status::service_unavailable: stream << "503 Service Unavailable"; break;
        case boost::asynchronous::http::status::gateway_timeout: stream << "504 Gateway Timeout"; break;
        case boost::asynchronous::http::status::http_version_not_supported: stream << "505 HTTP Version Not Supported"; break;
        case boost::asynchronous::http::status::variant_also_negotiates: stream << "506 Variant Also Negotiates"; break;
        case boost::asynchronous::http::status::not_extended: stream << "510 Not Extended"; break;
        case boost::asynchronous::http::status::network_authentication_required: stream << "511 Network Authentication Required"; break;

        default: stream << "599 Unknown Error"; break;
    }
    return stream;
}

boost::asynchronous::http::status_category categorize(boost::asynchronous::http::status code)
{
    switch (code)
    {
        case boost::asynchronous::http::status::continue_: [[fallthrough]];
        case boost::asynchronous::http::status::switching_protocols: [[fallthrough]];
        case boost::asynchronous::http::status::processing: [[fallthrough]];
        case boost::asynchronous::http::status::early_hints:
            return boost::asynchronous::http::status_category::informational;

        case boost::asynchronous::http::status::ok: [[fallthrough]];
        case boost::asynchronous::http::status::created: [[fallthrough]];
        case boost::asynchronous::http::status::accepted: [[fallthrough]];
        case boost::asynchronous::http::status::non_authoritative_information: [[fallthrough]];
        case boost::asynchronous::http::status::no_content: [[fallthrough]];
        case boost::asynchronous::http::status::reset_content: [[fallthrough]];
        case boost::asynchronous::http::status::partial_content:
            return boost::asynchronous::http::status_category::success;

        case boost::asynchronous::http::status::multiple_choices: [[fallthrough]];
        case boost::asynchronous::http::status::moved_permanently: [[fallthrough]];
        case boost::asynchronous::http::status::found: [[fallthrough]];
        case boost::asynchronous::http::status::see_other: [[fallthrough]];
        case boost::asynchronous::http::status::not_modified: [[fallthrough]];
        case boost::asynchronous::http::status::use_proxy: [[fallthrough]];
        case boost::asynchronous::http::status::switch_proxy: [[fallthrough]];
        case boost::asynchronous::http::status::temporary_redirect: [[fallthrough]];
        case boost::asynchronous::http::status::permanent_redirect:
            return boost::asynchronous::http::status_category::redirect;

        case boost::asynchronous::http::status::bad_request: [[fallthrough]];
        case boost::asynchronous::http::status::unauthorized: [[fallthrough]];
        case boost::asynchronous::http::status::payment_required: [[fallthrough]];
        case boost::asynchronous::http::status::forbidden: [[fallthrough]];
        case boost::asynchronous::http::status::not_found: [[fallthrough]];
        case boost::asynchronous::http::status::method_not_allowed: [[fallthrough]];
        case boost::asynchronous::http::status::not_acceptable: [[fallthrough]];
        case boost::asynchronous::http::status::proxy_authentication_required: [[fallthrough]];
        case boost::asynchronous::http::status::request_timeout: [[fallthrough]];
        case boost::asynchronous::http::status::conflict: [[fallthrough]];
        case boost::asynchronous::http::status::gone: [[fallthrough]];
        case boost::asynchronous::http::status::length_required: [[fallthrough]];
        case boost::asynchronous::http::status::precondition_failed: [[fallthrough]];
        case boost::asynchronous::http::status::payload_too_large: [[fallthrough]];
        case boost::asynchronous::http::status::uri_too_long: [[fallthrough]];
        case boost::asynchronous::http::status::unsupported_media_type: [[fallthrough]];
        case boost::asynchronous::http::status::range_not_satisfiable: [[fallthrough]];
        case boost::asynchronous::http::status::expectation_failed: [[fallthrough]];
        case boost::asynchronous::http::status::misdirected_request: [[fallthrough]];
        case boost::asynchronous::http::status::too_early: [[fallthrough]];
        case boost::asynchronous::http::status::upgrade_required: [[fallthrough]];
        case boost::asynchronous::http::status::precondition_required: [[fallthrough]];
        case boost::asynchronous::http::status::too_many_requests: [[fallthrough]];
        case boost::asynchronous::http::status::request_header_fields_too_large: [[fallthrough]];
        case boost::asynchronous::http::status::unavailable_for_legal_reasons:
            return boost::asynchronous::http::status_category::client_error;

        case boost::asynchronous::http::status::internal_server_error: [[fallthrough]];
        case boost::asynchronous::http::status::not_implemented: [[fallthrough]];
        case boost::asynchronous::http::status::bad_gateway: [[fallthrough]];
        case boost::asynchronous::http::status::service_unavailable: [[fallthrough]];
        case boost::asynchronous::http::status::gateway_timeout: [[fallthrough]];
        case boost::asynchronous::http::status::http_version_not_supported: [[fallthrough]];
        case boost::asynchronous::http::status::variant_also_negotiates: [[fallthrough]];
        case boost::asynchronous::http::status::not_extended: [[fallthrough]];
        case boost::asynchronous::http::status::network_authentication_required:
            return boost::asynchronous::http::status_category::server_error;

        default:
            return boost::asynchronous::http::status_category::unknown;
    }
}

std::ostream& operator<<(std::ostream& stream, boost::asynchronous::http::version version)
{
    switch (version)
    {
        case boost::asynchronous::http::version::httpv10: stream << "HTTP/1.0"; break;
        case boost::asynchronous::http::version::httpv11: stream << "HTTP/1.1"; break;
        default: stream << "HTTP/1.1"; break; // Fallback for future versions
    }
    return stream;
}



// ==========================================================================
// Responses

response::response(boost::asynchronous::http::content content, boost::asynchronous::http::status status, std::map<std::string, std::string> extra_headers, boost::asynchronous::http::version version)
    : version(version)
    , status(status)
    , headers {{ "Content-Length", std::to_string(content.body.size()) }, { "Content-Type", content.type.empty() ? BOOST_ASYNCHRONOUS_HTTP_DEFAULT_CONTENT_TYPE : content.type }}
    , body(std::move(content.body))
{
    for (const auto& pair : extra_headers)
        headers.insert(pair);
}

void response::set_content(boost::asynchronous::http::content content)
{
    body = std::move(content.body);
    headers["Content-Length"] = std::to_string(body.size());
    headers["Content-Type"] = content.type.empty() ? BOOST_ASYNCHRONOUS_HTTP_DEFAULT_CONTENT_TYPE : content.type;
}



// ==========================================================================
// Requests

boost::asynchronous::http::response request::make_response(boost::asynchronous::http::status status, boost::asynchronous::http::content&& content, std::map<std::string, std::string>&& headers) const
{
    return boost::asynchronous::http::response { std::move(content), status, std::move(headers), version };
}

boost::asynchronous::http::response request::make_empty_response(boost::asynchronous::http::status status, std::map<std::string, std::string>&& headers) const
{
    boost::asynchronous::http::response response { boost::asynchronous::http::content {}, status, std::move(headers), version };
    response.headers.erase("Content-Length");
    response.headers.erase("Content-Type");
    return response;
}




// ==========================================================================
// HTTP parser

boost::asynchronous::http::status parser::feed(std::string&& line)
{
    switch (m_state)
    {
        case parsing_state::request_line:
        {
            std::size_t methodSeparator = line.find(' ');
            if (methodSeparator == std::string::npos || methodSeparator == 0)
                return boost::asynchronous::http::status::bad_request;
            std::string method = line.substr(0, methodSeparator);
            std::transform(method.cbegin(), method.cend(), method.begin(), [](char ch) { return std::toupper<char>(ch, std::locale()); });
            m_request.method = std::move(method);
            if (m_request.method.empty())
                return boost::asynchronous::http::status::bad_request;

            std::size_t uriSeparator = line.find_first_of(" \r", methodSeparator + 1);
            if (uriSeparator == std::string::npos || uriSeparator <= methodSeparator)
                return boost::asynchronous::http::status::bad_request;
            m_request.uri = line.substr(methodSeparator + 1, uriSeparator - methodSeparator - 1);
            if (m_request.uri.empty())
                return boost::asynchronous::http::status::bad_request;

            std::string versionMarker = line.substr(uriSeparator + 1);
            if (versionMarker.empty())
                return boost::asynchronous::http::status::http_version_not_supported; // Don't deal with HTTP/0.9's quirks anymore.
            else if (versionMarker == "HTTP/1.0")
                m_request.version = boost::asynchronous::http::version::httpv10;
            else if (versionMarker == "HTTP/1.1")
                m_request.version = boost::asynchronous::http::version::httpv11;
            else
                m_request.version = boost::asynchronous::http::version::unknown; // Treat unknown as if it were HTTP/1.1, because who knows what comes in the future...

            m_state = parsing_state::headers;
            return boost::asynchronous::http::status::continue_processing;
        }

        case parsing_state::headers:
        {
            if (line == "\r" || line.empty())
            {
                m_state = parsing_state::headers_completed;
                return boost::asynchronous::http::status::continue_processing;
            }

            std::size_t valueSeparator = line.find(':');
            if (valueSeparator == std::string::npos || valueSeparator == 0)
                return boost::asynchronous::http::status::bad_request;
            std::size_t valueStart = line.find_first_not_of(" ", valueSeparator + 1);
            if (valueStart == std::string::npos || valueStart <= valueSeparator)
                return boost::asynchronous::http::status::bad_request;

            std::string headerName = line.substr(0, valueSeparator);
            std::transform(headerName.cbegin(), headerName.cend(), headerName.begin(), [](char ch) { return std::tolower<char>(ch, std::locale()); }); // This is not unicode-sensitive, but we should only have ASCII header names anyways; values are not affected.
            std::size_t eolSpace = line.find_last_not_of(" \r\n\t\v\f");
            std::string headerValue = line.substr(valueStart, eolSpace - valueStart + 1);

            // Per RFC2616/4.2, convert multiple headers to a comma-separated list (if that was invalid in the first place, the request is ill-formed)
            auto iterator = m_request.headers.find(headerName);
            if (iterator != m_request.headers.end())
                iterator->second += ", " + headerValue;
            else
                m_request.headers.emplace(std::move(headerName), std::move(headerValue));
            return boost::asynchronous::http::status::continue_processing;
        }

        case parsing_state::headers_completed:
            return boost::asynchronous::http::status::internal_server_error;
    }
}

boost::asynchronous::http::request parser::finalize() const
{
    return m_request;
}



// ==========================================================================
// Sockets

socket::socket(std::shared_ptr<raw_socket> underlying)
    : m_socket(std::move(underlying))
    , m_is_raw(true)
{}

socket::socket(std::shared_ptr<wrapped_socket> underlying)
    : m_socket(std::move(underlying))
    , m_is_raw(false)
{}

bool socket::is_raw() const
{
    return m_is_raw;
}

boost::asynchronous::http::socket::raw_socket& socket::raw()
{
    BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(
        if (m_is_raw)
            return *get<raw_socket>();
        else
            return get<wrapped_socket>()->next_layer();
    )
    BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK(
        return *get<raw_socket>();
    )
}



// ==========================================================================
// Acceptor

acceptor::acceptor(boost::asynchronous::any_weak_scheduler<> scheduler,
                   const std::string& address,
                   unsigned int port,
                   const boost::asynchronous::http::security& security,
                   std::function<void(boost::asynchronous::http::socket&&)> connection_handler)
    : boost::asynchronous::trackable_servant<>(scheduler)
    , m_acceptor(*boost::asynchronous::get_io_service<>())
    , m_raw_socket(*boost::asynchronous::get_io_service<>())
    , m_connection_handler(std::move(connection_handler))
    BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(, m_context { BOOST_ASYNCHRONOUS_HTTP_DEFAULT_SSL_METHOD })
{
    boost::asio::ip::tcp::resolver resolver(*boost::asynchronous::get_io_service<>());
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({ address, std::to_string(port) });

    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address { true });
    m_acceptor.bind(endpoint);
    m_acceptor.listen();

    BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(
        m_use_https = !!security.https;
        if (m_use_https)
        {
            m_context.set_options(BOOST_ASYNCHRONOUS_HTTP_DEFAULT_SSL_FLAGS);
            m_context.use_certificate_chain_file(security.https.value().certificate_chain_file);
            m_context.use_private_key_file(security.https.value().private_key_file, boost::asio::ssl::context::pem);
        }
        else
        {
            m_use_https = false;
        }
    )
    BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK(
        if (!!security.https)
        {
            BOOST_ASYNCHRONOUS_HTTP_LOG_ERROR("HTTPS options provided, but SSL is disabled (BOOST_ASYNCHRONOUS_HTTP_NO_SSL)");
            throw std::logic_error("Cannot enable HTTPS if SSL support is disabled at compile time (BOOST_ASYNCHRONOUS_HTTP_NO_SSL)");
        }
    )

    accept();
}

acceptor::~acceptor()
{
    m_acceptor.close();
}

void acceptor::accept()
{
    if (!m_acceptor.is_open())
        return;
    m_acceptor.async_accept(
        m_raw_socket,
        make_safe_callback(
            [this](boost::system::error_code ec)
            {
                if (!m_acceptor.is_open())
                    return;
                if (!ec)
                {
                    m_raw_socket.set_option(boost::asio::ip::tcp::no_delay { true });

                    BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(
                        if (m_use_https)
                        {
                            auto ssl_socket = std::make_shared<boost::asynchronous::http::socket::wrapped_socket>(*boost::asynchronous::get_io_service<>(), m_context);
                            ssl_socket->lowest_layer() = std::move(m_raw_socket);
                            m_connection_handler(boost::asynchronous::http::socket { std::move(ssl_socket) });
                        }
                        else
                        {
                            m_connection_handler(boost::asynchronous::http::socket { std::make_shared<boost::asynchronous::http::socket::raw_socket>(std::move(m_raw_socket)) });
                        }
                    )
                    BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK(
                        m_connection_handler(boost::asynchronous::http::socket { std::make_shared<boost::asynchronous::http::socket::raw_socket>(std::move(m_raw_socket)) });
                    )
                }
                accept();
            },
            "boost::asynchronous::http::acceptor: on_accepted",
            BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
        )
    );
}

acceptor_proxy::acceptor_proxy(boost::asynchronous::any_shared_scheduler_proxy<> scheduler, const std::string& address, unsigned int port, const boost::asynchronous::http::security& security, std::function<void(boost::asynchronous::http::socket&&)> connection_handler)
    : boost::asynchronous::servant_proxy<boost::asynchronous::http::acceptor_proxy, boost::asynchronous::http::acceptor>(scheduler, address, port, security, std::move(connection_handler))
{}



// ==========================================================================
// Connections

connection::connection(boost::asynchronous::any_weak_scheduler<boost::asynchronous::any_callable> asio_scheduler, boost::asynchronous::http::socket&& socket)
    : boost::asynchronous::trackable_servant<boost::asynchronous::any_callable, boost::asynchronous::any_callable>(asio_scheduler)
    , m_socket(std::move(socket))
    , m_endpoint(m_socket.raw().remote_endpoint())
{}

void connection::start(std::function<void(boost::asynchronous::http::status, boost::asynchronous::http::request&&)> callback)
{
    BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(
        if (!m_socket.is_raw())
        {
            m_socket.get<boost::asynchronous::http::socket::wrapped_socket>()->async_handshake(
                boost::asio::ssl::stream_base::server,
                make_safe_callback(
                    [this, callback = std::move(callback), keepalive = m_socket, remote = m_endpoint](boost::system::error_code ec)
                    {
                        if (!!ec)
                        {
                            BOOST_ASYNCHRONOUS_HTTP_LOG_INFO("Handshake with " << remote << " failed: " << ec.message());
                            callback(boost::asynchronous::http::status::ssl_error, boost::asynchronous::http::request {});
                        }
                        else
                        {
                            read_header(std::move(callback));
                        }
                    },
                    "boost::asynchronous::http::connection: on_handshake_completed",
                    BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
                )
            );
        }
        else
        {
            read_header(std::move(callback));
        }
    )
    BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK(
        read_header(std::move(callback));
    )
}

void connection::send(boost::asynchronous::http::response&& response, std::function<void(bool)> callback)
{
    std::ostringstream header_stream;

    // Write status
    header_stream << response.version << " " << response.status << "\r\n";

    // Write headers
    for (const auto& pair : response.headers)
        header_stream << pair.first << ": " << pair.second << "\r\n";

    // Write body, if any
    header_stream << "\r\n";

    // Send the headers alongside the body
    std::shared_ptr<std::string> headers = std::make_shared<std::string>(header_stream.str());
    std::shared_ptr<std::string> body = std::make_shared<std::string>(std::move(response.body));

    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(*headers));
    buffers.push_back(boost::asio::buffer(*body));

    m_socket.write(
        buffers,
        [headers, body, keepalive = m_socket, remote = m_endpoint, callback = std::move(callback)](boost::system::error_code ec, std::size_t /* bytes_transferred */) mutable
        {
            // We don't care in which thread this lambda happens, we'll just forward this to the server thread's callback instantly anyways.
            if (!!ec)
                BOOST_ASYNCHRONOUS_HTTP_LOG_ERROR("Failed to send response to " << remote);
            callback(!ec);
        }
    );
}

void connection::read_header(std::function<void(boost::asynchronous::http::status, boost::asynchronous::http::request&&)> callback)
{
    auto buffer = std::make_shared<boost::asio::streambuf>(BOOST_ASYNCHRONOUS_HTTP_MAX_HEADER_LENGTH);
    m_socket.read_until(
        *buffer,
        boost::regex(R"((\r\n|\n){2})"), // TODO: To strictly follow HTTP spec, we should treat \r the same as \r\n, but getline later doesn't do that either, so we don't do it here (also, noone is actually mad enough to send \r only nowadays).
        make_safe_callback(
            [this, callback, buffer, keepalive = m_socket, remote = m_endpoint](boost::system::error_code ec, size_t /* bytes_transferred */) mutable
            {
                if (!!ec || buffer->size() == 0)
                {
                    BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, remote, "No data received: " << ec.message());
                    callback(boost::asynchronous::http::status::bad_request, boost::asynchronous::http::request {});
                }
                else if (buffer->size() >= BOOST_ASYNCHRONOUS_HTTP_MAX_HEADER_LENGTH)
                {
                    BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::request_header_fields_too_large, remote, "Header size exceeds limit");
                    callback(boost::asynchronous::http::status::request_header_fields_too_large, boost::asynchronous::http::request {});
                }
                else
                {
                    // Parse the header to find out how much body we need to read, or to reject the request entirely
                    std::istream stream(buffer.get());
                    std::string line;
                    boost::asynchronous::http::parser parser;
                    while (std::getline(stream, line))
                    {
                        auto status = parser.feed(std::move(line));
                        if (status != boost::asynchronous::http::status::continue_processing)
                        {
                            BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(status, remote, "Malformed HTTP header");
                            callback(status, boost::asynchronous::http::request {});
                            return;
                        }
                        if (line == "\r" || line.empty())
                            break;
                    }
                    boost::asynchronous::http::request request = parser.finalize();
                    request.remote = std::move(remote);
                    request.did_send_continue = false;

                    // Our buffer should be empty now; otherwise, hold on to the data.
                    if (buffer->size() != 0)
                        m_retained = std::move(buffer);

                    // Read and parse the response body (technically, this is only necessary for some request methods, but we do want to enable custom methods, e.g. for WebDAV-like protocols)
                    // We do not do this for request methods that are in BOOST_ASYNCHRONOUS_HTTP_BODYLESS_METHODS (because browsers don't indicate body length if there is no body)
                    if (BOOST_ASYNCHRONOUS_HTTP_BODYLESS_METHODS.count(request.method) == 0)
                        read_body(std::move(request), std::move(callback));
                    else
                        callback(boost::asynchronous::http::status::continue_processing, std::move(request));
                }
            },
            "boost::asynchronous::http::connection: on_header_read",
            BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
        )
    );
}

void connection::read_body(boost::asynchronous::http::request&& request, std::function<void(boost::asynchronous::http::status, boost::asynchronous::http::request&&)> callback)
{
    // If the client transmitted an Expect: 100-continue, we should answer with 100 Continue before receiving the header.
    auto expect_iterator = request.headers.find("expect");
    if (expect_iterator != request.headers.end() && !request.did_send_continue)
    {
        boost::regex should_continue(R"([[:space:]]|^)100-continue[[:space::]*($|,)", boost::regex::icase);
        if (boost::regex_search(expect_iterator->second, should_continue))
        {
            request.did_send_continue = true;
            send(
                request.make_empty_response(boost::asynchronous::http::status::continue_),
                make_safe_callback(
                    [this, request = std::move(request), callback = std::move(callback)](bool successful) mutable
                    {
                        if (!successful)
                        {
                            BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, request.remote, "No data received after sending 100 Continue");
                            callback(boost::asynchronous::http::status::bad_request, boost::asynchronous::http::request {});
                            return;
                        }
                        else
                        {
                            // Continue reading the actual body.
                            read_body(std::move(request), std::move(callback));
                        }
                    },
                    "boost::asynchronous::http::connection: on_continued",
                    BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
                )
            );
            return;
        }
    }

    // Check how the request body is delimited
    auto transfer_encoding_iterator = request.headers.find("transfer-encoding");
    auto content_length_iterator = request.headers.find("content-length");
    if (transfer_encoding_iterator != request.headers.end())
    {
        boost::regex chunked(R"([[:space:]]|^)chunked[[:space::]*($|,)", boost::regex::icase);
        if (boost::regex_search(transfer_encoding_iterator->second, chunked))
        {
            request.size = 0;
            read_chunk(std::move(request), std::move(callback));
            return;
        }
    }
    if (content_length_iterator != request.headers.end())
    {
        try
        {
            std::size_t pos;
            request.size = std::stoull(content_length_iterator->second, &pos);
            if (!std::all_of(content_length_iterator->second.cbegin() + pos, content_length_iterator->second.cend(), [](char ch) { return std::isspace(ch); }))
            {
                BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, request.remote, "Invalid characters at end of Content-Length header");
                callback(boost::asynchronous::http::status::bad_request, std::move(request));
                return;
            }
        }
        catch (std::invalid_argument const& e)
        {
            BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, request.remote, "Malformed Content-Length header");
            callback(boost::asynchronous::http::status::bad_request, std::move(request));
            return;
        }
        catch (std::out_of_range const& e)
        {
            BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, request.remote, "Content-Length out of range");
            callback(boost::asynchronous::http::status::bad_request, std::move(request));
            return;
        }

        // Read up to Content-Length bytes, as long as that size is sane.
        if (request.size > BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_LENGTH)
        {
            BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::payload_too_large, request.remote, "Content-Length of " << request.size << " bytes is larger than configured BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_LENGTH (" << BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_LENGTH << " bytes)");
            callback(boost::asynchronous::http::status::payload_too_large, std::move(request));
            return;
        }

        // Clear retained buffers
        auto buffer = std::make_shared<boost::asio::streambuf>(request.size);
        if (m_retained)
        {
            move_streambuf_contents(*buffer, *m_retained);
            m_retained.reset();
        }

        // Read until full
        m_socket.read(
            *buffer,
            make_safe_callback(
                [this, callback, buffer, keepalive = m_socket, request = std::move(request)](boost::system::error_code ec, size_t /* bytes_transferred */) mutable
                {
                    if (!!ec || buffer->size() == 0)
                    {
                        BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, request.remote, "No data received: " << ec.message());
                        callback(boost::asynchronous::http::status::bad_request, boost::asynchronous::http::request {});
                        return;
                    }
                    else if (buffer->size() != request.size)
                    {
                        // The request appears to be incomplete (async_read should not allow this, but we handle this case anyways)
                        BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::request_timeout, request.remote, "Incomplete request (" << buffer->size() << " of " << request.size << " bytes received)");
                        callback(boost::asynchronous::http::status::request_timeout, boost::asynchronous::http::request {});
                        return;
                    }
                    else
                    {
                        // This is Content-Length-based data, so we can instantly store the buffer contents in the request.
                        std::istream stream(buffer.get());
                        request.body.resize(request.size);
                        stream.read(const_cast<char *>(request.body.data()), request.size); // TODO: const_cast is only necessary until C++17.
                        callback(boost::asynchronous::http::status::continue_processing, std::move(request));
                    }
                },
                "boost::asynchronous::http::connection: on_body_read_by_content_length",
                BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
            )
        );
        return;
    }

    // No length, and no chunked transfer encoding. We would like one of those!
    BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::length_required, request.remote, "Transfer-Encoding is not 'chunked', but no Content-Length specified");
    callback(boost::asynchronous::http::status::length_required, std::move(request));
}

void connection::read_chunk(boost::asynchronous::http::request&& request, std::function<void(boost::asynchronous::http::status, boost::asynchronous::http::request&&)> callback)
{
    // Clear retained buffers
    auto buffer = std::make_shared<boost::asio::streambuf>(BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_HEADER_LENGTH);
    if (m_retained)
    {
        move_streambuf_contents(*buffer, *m_retained);
        m_retained.reset();
    }
    // Read chunk size
    m_socket.read_until(
        *buffer,
        boost::regex(R"((\r\n|\n)?\d+(\r\n|\n))"), // TODO: To strictly follow HTTP spec, we should treat \r the same as \r\n, but getline later doesn't do that either, so we don't do it here (also, noone is actually mad enough to send \r only nowadays).
        make_safe_callback(
            [this, callback, buffer, keepalive = m_socket, request = std::move(request)](boost::system::error_code ec, size_t /* bytes_transferred */) mutable
            {
                if (!!ec || buffer->size() == 0)
                {
                    BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, request.remote, "No data received: " << ec.message());
                    callback(boost::asynchronous::http::status::bad_request, boost::asynchronous::http::request {});
                }
                else if (buffer->size() >= BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_HEADER_LENGTH)
                {
                    BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, request.remote, "Length of chunk size exceeds limit");
                    callback(boost::asynchronous::http::status::bad_request, boost::asynchronous::http::request {});
                }
                else
                {
                    // Get the length of the chunk.
                    std::size_t chunk_size;
                    std::istream stream(buffer.get());
                    stream >> chunk_size;

                    if (chunk_size >= BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_LENGTH)
                    {
                        BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::payload_too_large, request.remote, "Chunk size of " << chunk_size << " bytes is larger than configured BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_LENGTH (" << BOOST_ASYNCHRONOUS_HTTP_MAX_CHUNK_LENGTH << " bytes)");
                        callback(boost::asynchronous::http::status::payload_too_large, std::move(request));
                        return;
                    }

                    if (chunk_size == 0)
                    {
                        // End of transferred data.
                        callback(boost::asynchronous::http::status::continue_processing, std::move(request));
                        return;
                    }

                    // Read that data
                    auto data_buffer = std::make_shared<boost::asio::streambuf>(chunk_size);
                    if (buffer->size() != 0)
                        move_streambuf_contents(*data_buffer, *buffer); // Copy remaining data from read_until

                    m_socket.read(
                        *data_buffer,
                        make_safe_callback(
                            [this, callback, chunk_size, data_buffer, keepalive = m_socket, request = std::move(request)](boost::system::error_code ec, size_t /* bytes_transferred */) mutable
                            {
                                if (!!ec || data_buffer->size() == 0)
                                {
                                    BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::bad_request, request.remote, "No data received: " << ec.message());
                                    callback(boost::asynchronous::http::status::bad_request, boost::asynchronous::http::request {});
                                    return;
                                }
                                else if (data_buffer->size() != chunk_size)
                                {
                                    // The request appears to be incomplete (async_read should not allow this, but we handle this case anyways)
                                    BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(boost::asynchronous::http::status::request_timeout, request.remote, "Incomplete chunk in request (" << data_buffer->size() << " of " << chunk_size << " bytes received)");
                                    callback(boost::asynchronous::http::status::request_timeout, boost::asynchronous::http::request {});
                                    return;
                                }
                                else
                                {
                                    // Append to the request body
                                    std::istream stream(data_buffer.get());
                                    request.body.resize(request.size + chunk_size);
                                    stream.read(const_cast<char *>(request.body.data()) + request.size, chunk_size); // TODO: const_cast is only necessary until C++17.
                                    request.size += chunk_size;

                                    // Read the next chunk
                                    read_chunk(std::move(request), std::move(callback));
                                }
                            },
                            "boost::asynchronous::http::connection: on_chunk_read",
                            BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
                        )
                    );
                }
            },
            "boost::asynchronous::http::connection: on_chunk_size_read",
            BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
        )
    );
}

connection_proxy::connection_proxy(boost::asynchronous::any_shared_scheduler_proxy<> scheduler, boost::asynchronous::http::socket&& socket)
    : boost::asynchronous::servant_proxy<boost::asynchronous::http::connection_proxy, boost::asynchronous::http::connection>(scheduler, std::move(socket))
{}



// ==========================================================================
// Server

server::server(boost::asynchronous::http::weak_scheduler scheduler,
               boost::asynchronous::any_shared_scheduler_proxy<> asio_scheduler,
               const std::string& address,
               unsigned int port,
               const boost::asynchronous::http::security& security)
    : boost::asynchronous::http::trackable_servant(scheduler)
    , m_address(address)
    , m_port(port)
    , m_asio_scheduler(asio_scheduler)
    , m_acceptor_proxy {
            m_asio_scheduler,
            m_address,
            m_port,
            security,
            make_safe_callback([this](boost::asynchronous::http::socket&& socket) { handle_connect(std::move(socket)); }, "boost::asynchronous::http::server: on_connect", BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY)
        }
{
    // Set up default error pages.
    for (int error = static_cast<int>(boost::asynchronous::http::status::minimum_external_status); error < static_cast<int>(boost::asynchronous::http::status::maximum_external_status); ++error)
    {
        auto status = static_cast<boost::asynchronous::http::status>(error);
        auto category = boost::asynchronous::http::categorize(status);
        if (category == boost::asynchronous::http::status_category::client_error || category == boost::asynchronous::http::status_category::server_error)
        {
            std::ostringstream pageBuilder;
            pageBuilder << "<!DOCTYPE html><html><head><title>" << status << "</title></head><body><h1>" << status << "</h1><hr /></body></html>\n";
            m_error_pages[status] = boost::asynchronous::http::content { pageBuilder.str(), "text/html" };
        }
    }
}

void server::add_route(std::vector<std::string>&& methods, boost::regex&& uri, route&& route)
{
    m_routes.emplace_back(std::move(uri), std::move(methods), std::move(route));
}

void server::set_error_page(boost::asynchronous::http::status status, boost::asynchronous::http::content&& content)
{
    m_error_pages[status] = std::move(content);
}

void server::handle_connect(boost::asynchronous::http::socket&& socket)
{
    auto connection = std::make_shared<boost::asynchronous::http::connection_proxy>(m_asio_scheduler, std::move(socket));
    m_pending.insert(connection);

    // Use a weak_ptr so that the connection does not keep itself alive
    std::weak_ptr<boost::asynchronous::http::connection_proxy> weak_connection(connection);
    connection->start(
        make_safe_callback(
            [this, weak_connection](boost::asynchronous::http::status status, boost::asynchronous::http::request&& request)
            {
                if (status == boost::asynchronous::http::status::ssl_error)
                    handle_completed(weak_connection);
                else
                    handle_request(weak_connection, status, std::move(request));
            },
            "boost::asynchronous::http::server: on_connection_started",
            BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
        )
    );
}

void server::handle_request(std::weak_ptr<boost::asynchronous::http::connection_proxy> weak_connection, boost::asynchronous::http::status status, boost::asynchronous::http::request&& request)
{
    auto connection = weak_connection.lock();
    if (!connection)
        // Ignore TCP errors
        return;

    if (status != boost::asynchronous::http::status::continue_processing)
    {
        // Reply with the error only.
        send_final_response(connection, request.make_response(status));
        return;
    }

    // route the request (but strip off URI parameters for routing)
    std::string without_params = request.uri.substr(0, request.uri.find('?'));
    boost::asynchronous::http::status routing_status = boost::asynchronous::http::status::not_found;
    for (const auto& route : m_routes)
    {
        if (!boost::regex_match(without_params, std::get<0>(route), boost::regex_constants::match_any))
            continue;
        if (std::find(std::get<1>(route).cbegin(), std::get<1>(route).cend(), request.method) == std::get<1>(route).cend())
        {
            // Matched URI, but not the method
            routing_status = boost::asynchronous::http::status::method_not_allowed;
            continue;
        }
        // Keep some information for logging
        auto remote = request.remote;
        std::string descriptor = request.method + " " + request.uri;
        // Delegate the route. This can (or rather should) be going into a proxy so we don't block the server.
        std::get<2>(route)(
            connection,
            std::move(request),
            make_safe_callback(
                [this, weak_connection, remote, descriptor](boost::asynchronous::http::response&& response)
                {
                    // Routing can return no_response if it doesn't want us to deal with sending the response (and instead use connection->send manually for some other protocol)
                    if (response.status == boost::asynchronous::http::status::no_response)
                        handle_completed(weak_connection);
                    else
                    {
                        auto connection = weak_connection.lock();
                        if (!connection)
                            return;
                        BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(response.status, remote, descriptor);
                        send_final_response(connection, std::move(response));
                    }
                },
                "boost::asynchronous::http::server: on_routed",
                BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
            )
        );
        return;
    }

    // Routing failed.
    BOOST_ASYNCHRONOUS_HTTP_LOG_RESPONSE(routing_status, request.remote, request.method << " " << request.uri);
    send_final_response(connection, request.make_response(routing_status));
}

void server::handle_completed(std::weak_ptr<boost::asynchronous::http::connection_proxy> weak_connection)
{
    auto connection = weak_connection.lock();
    if (!connection)
        return;
    m_pending.erase(connection);
}

void server::send_final_response(std::shared_ptr<boost::asynchronous::http::connection_proxy> connection, boost::asynchronous::http::response&& response)
{
    std::weak_ptr<boost::asynchronous::http::connection_proxy> weak_connection(connection);
    if (response.body.empty())
    {
        // Replace the response with the configured error page for the status code
        auto page_iterator = m_error_pages.find(response.status);
        if (page_iterator != m_error_pages.end())
            response.set_content(page_iterator->second);
    }
    connection->send(
        std::move(response),
        make_safe_callback(
            [this, weak_connection](bool /*successful*/)
            {
                handle_completed(weak_connection);
            },
            "boost::asynchronous::http::server: on_final_response_sent",
            BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
        )
    );
}

server_proxy::server_proxy(boost::asynchronous::http::shared_scheduler scheduler, boost::asynchronous::any_shared_scheduler_proxy<> asio_scheduler, const std::string& address, unsigned int port, const boost::asynchronous::http::security& security)
    : boost::asynchronous::http::servant_proxy<server_proxy, server>(scheduler, asio_scheduler, address, port, security)
{}

void server_proxy::add_route(std::vector<std::string> methods, boost::regex uri, boost::asynchronous::http::server::route route)
{
    add_route<std::vector<std::string>&&, boost::regex&&, boost::asynchronous::http::server::route&&>(std::move(methods), std::move(uri), std::move(route));
}

void server_proxy::add_route(std::vector<std::string> methods, std::string uri, boost::asynchronous::http::server::route route)
{
    add_route<std::vector<std::string>&&, boost::regex&&, boost::asynchronous::http::server::route&&>(std::move(methods), boost::regex(std::move(uri)), std::move(route));
}

}
}
}
