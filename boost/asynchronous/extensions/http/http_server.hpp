#include <map>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/asynchronous/any_shared_scheduler_proxy.hpp>
#include <boost/asynchronous/extensions/asio/tss_asio.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <boost/variant.hpp>

#ifndef BOOST_ASYNCHRONOUS_HTTP_JOB_TYPE
# define BOOST_ASYNCHRONOUS_HTTP_JOB_TYPE BOOST_ASYNCHRONOUS_DEFAULT_JOB
#endif

#ifndef BOOST_ASYNCHRONOUS_HTTP_NO_SSL
# include <boost/asio/ssl.hpp>
# ifndef BOOST_ASYNCHRONOUS_HTTP_DEFAULT_SSL_METHOD
#  define BOOST_ASYNCHRONOUS_HTTP_DEFAULT_SSL_METHOD boost::asio::ssl::context::method::tlsv12 // TODO: Upgrage this to TLS v1.3 (however, this would increase the required Boost version)
# endif
# ifndef BOOST_ASYNCHRONOUS_HTTP_DEFAULT_SSL_FLAGS
#  define BOOST_ASYNCHRONOUS_HTTP_DEFAULT_SSL_FLAGS  (boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3 | boost::asio::ssl::context::no_tlsv1 | boost::asio::ssl::context::single_dh_use)
# endif
# define BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(...) __VA_ARGS__
# define BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK(...)
#else
# define BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(...)
# define BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK(...) __VA_ARGS__
#endif

// If you want to use multiqueue schedulers for something in here, this is the place to adjust priorities
#ifndef BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY
# define BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY 0
#endif

namespace boost { namespace asynchronous { namespace http
{
    
// Logging utilities
enum class log_severity
{
    everything, // Fallback placeholder
    info,
    error,
    nothing
};

// Typedefs to propagate the selected job type without having to type quite as much.
// Note that we need to use the default job type in the asio_scheduler - we cannot fetch diagnostics from any_loggable, and any_serializable does not make sense.
using trackable_servant = boost::asynchronous::trackable_servant<BOOST_ASYNCHRONOUS_HTTP_JOB_TYPE, BOOST_ASYNCHRONOUS_HTTP_JOB_TYPE>;
using weak_scheduler = boost::asynchronous::any_weak_scheduler<BOOST_ASYNCHRONOUS_HTTP_JOB_TYPE>;
using shared_scheduler = boost::asynchronous::any_shared_scheduler_proxy<BOOST_ASYNCHRONOUS_HTTP_JOB_TYPE>;
template <typename Proxy, typename Servant> using servant_proxy = boost::asynchronous::servant_proxy<Proxy, Servant, BOOST_ASYNCHRONOUS_HTTP_JOB_TYPE>;

// HTTP status codes
enum class status
{
    // Internal codes
    continue_processing = 0,
    no_response = 1,
    ssl_error = 66,
    minimum_external_status = 99,
    
    // Official HTTP status codes
    continue_ = 100,
    switching_protocols = 101,
    processing = 102,
    early_hints = 103,
    
    ok = 200,
    created = 201,
    accepted = 202,
    non_authoritative_information = 203,
    no_content = 204,
    reset_content = 205,
    partial_content = 206,
    
    multiple_choices = 300,
    moved_permanently = 301,
    found = 302, // Will often, but not always, change the request to GET (like 303), but shouldn't (like 307). Prefer these two.
    see_other = 303,
    not_modified = 304,
    use_proxy = 305, // Often ignored.
    switch_proxy = 306, // No longer used.
    temporary_redirect = 307,
    permanent_redirect = 308, // Like 301, but do not change the request method.
    
    bad_request = 400,
    unauthorized = 401, // Authentication failed or has not been provided.
    payment_required = 402,
    forbidden = 403,
    not_found = 404,
    method_not_allowed = 405,
    not_acceptable = 406,
    proxy_authentication_required = 407,
    request_timeout = 408,
    conflict = 409,
    gone = 410,
    length_required = 411,
    precondition_failed = 412,
    payload_too_large = 413,
    uri_too_long = 414,
    unsupported_media_type = 415,
    range_not_satisfiable = 416,
    expectation_failed = 417,
    misdirected_request = 421,
    too_early = 425,
    upgrade_required = 426,
    precondition_required = 428,
    too_many_requests = 429, // Rate limit exceeded
    request_header_fields_too_large = 431,
    unavailable_for_legal_reasons = 451,
    
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503,
    gateway_timeout = 504,
    http_version_not_supported = 505,
    variant_also_negotiates = 506,
    not_extended = 510,
    network_authentication_required = 511,
    
    maximum_external_status
};

std::ostream& operator<<(std::ostream& stream, boost::asynchronous::http::status status);

enum class status_category
{
    unknown = 0,
    informational = 100,
    success = 200,
    redirect = 300,
    client_error = 400,
    server_error = 500
};

boost::asynchronous::http::status_category categorize(boost::asynchronous::http::status status);

enum class version
{
    unknown,
    httpv10,
    httpv11
};

std::ostream& operator<<(std::ostream& stream, boost::asynchronous::http::version version);

struct content
{
    std::string body;
    std::string type;
    
    operator bool() const { return !body.empty(); }
};

struct response
{
    boost::asynchronous::http::version version;
    boost::asynchronous::http::status status;
    std::map<std::string, std::string> headers;
    std::string body;
    
    explicit response(boost::asynchronous::http::content content, boost::asynchronous::http::status status = boost::asynchronous::http::status::ok, std::map<std::string, std::string> extraHeaders = {}, boost::asynchronous::http::version version = boost::asynchronous::http::version::httpv11);
    void set_content(boost::asynchronous::http::content content);
};

struct request
{
    std::string method;
    std::string uri;
    boost::asynchronous::http::version version;
    std::map<std::string, std::string> headers;
    std::string body;
    
    boost::asio::ip::tcp::endpoint remote;
    std::size_t size;
    bool did_send_continue;
    
    response make_response(boost::asynchronous::http::status status, boost::asynchronous::http::content&& content = {}, std::map<std::string, std::string>&& headers = {}) const;
    response make_empty_response(boost::asynchronous::http::status status, std::map<std::string, std::string>&& headers = {}) const;
};

class parser
{
public:
    boost::asynchronous::http::status feed(std::string&& line);
    boost::asynchronous::http::request finalize() const;
    
private:    
    enum class parsing_state
    {
        request_line,
        headers,
        headers_completed
    };

    boost::asynchronous::http::request m_request;
    parsing_state m_state;
};

struct security
{
    struct https_config
    {
        std::string certificate_chain_file;
        std::string private_key_file;
    };
    
    boost::optional<https_config> https;
};

class socket
{
public:
    using raw_socket = boost::asio::ip::tcp::socket;

    BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(using wrapped_socket = boost::asio::ssl::stream<raw_socket>;)
    BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK(private: struct wrapped_socket {}; public:)

    explicit socket(std::shared_ptr<raw_socket> underlying);
    explicit socket(std::shared_ptr<wrapped_socket> underlying);
    
    template <class T>
    std::shared_ptr<T> get()
    {
        return boost::get<std::shared_ptr<T>>(m_socket);
    }

#define BOOST_ASYNCHRONOUS_HTTP_IMPL_SOCKET_WRAP(name, free_function) \
    template <class... Arguments> \
    decltype(auto) name(Arguments&&... args) \
    { \
        /* Visiting with lambdas doesn't really work, so we do this instead... */ \
        BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL( \
            if (m_is_raw) \
                return free_function(*get<raw_socket>(), std::forward<Arguments>(args)...); \
            else \
                return free_function(*get<wrapped_socket>(), std::forward<Arguments>(args)...); \
        ) \
        BOOST_ASYNCHRONOUS_HTTP_NO_SSL_FALLBACK( \
            return free_function(*get<raw_socket>(), std::forward<Arguments>(args)...); \
        ) \
    }
    
    BOOST_ASYNCHRONOUS_HTTP_IMPL_SOCKET_WRAP(read, boost::asio::async_read)
    BOOST_ASYNCHRONOUS_HTTP_IMPL_SOCKET_WRAP(read_until, boost::asio::async_read_until)
    BOOST_ASYNCHRONOUS_HTTP_IMPL_SOCKET_WRAP(write, boost::asio::async_write)

#undef BOOST_ASYNCHRONOUS_HTTP_IMPL_SOCKET_WRAP
    
    bool is_raw() const;
    raw_socket& raw();
    
private:
    boost::variant<std::shared_ptr<wrapped_socket>, std::shared_ptr<raw_socket>> m_socket;
    bool m_is_raw;
};

class acceptor : public boost::asynchronous::trackable_servant<>
{
public:
    acceptor(boost::asynchronous::any_weak_scheduler<> scheduler, const std::string& address, unsigned int port, const boost::asynchronous::http::security& security, std::function<void(boost::asynchronous::http::socket&&)> connection_handler);
    ~acceptor();
    
private:
    void accept();
    
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asynchronous::http::socket::raw_socket m_raw_socket;
    std::function<void(boost::asynchronous::http::socket&&)> m_connection_handler;
    
    BOOST_ASYNCHRONOUS_HTTP_REQUIRES_SSL(
        bool m_use_https;
        boost::asio::ssl::context m_context;
    )
};

class acceptor_proxy : public boost::asynchronous::servant_proxy<boost::asynchronous::http::acceptor_proxy, boost::asynchronous::http::acceptor>
{
public:
    acceptor_proxy(boost::asynchronous::any_shared_scheduler_proxy<> scheduler, const std::string& address, unsigned int port, const boost::asynchronous::http::security& security, std::function<void(boost::asynchronous::http::socket&&)> connection_handler);
};

class connection : public boost::asynchronous::trackable_servant<>
{
public:
    connection(boost::asynchronous::any_weak_scheduler<> asio_scheduler, boost::asynchronous::http::socket&& socket);

    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;

    void start(std::function<void(boost::asynchronous::http::status, boost::asynchronous::http::request&&)> callback);
    void send(boost::asynchronous::http::response&& response, std::function<void(bool)> callback);
    void read_body(boost::asynchronous::http::request&& request, std::function<void(boost::asynchronous::http::status, boost::asynchronous::http::request&&)> callback); // Users can manually invoke this if they want to read a request body that was skipped by default (e.g. for GET requests)
    
private:
    void read_header(std::function<void(boost::asynchronous::http::status, boost::asynchronous::http::request&&)> callback);
    void read_chunk(boost::asynchronous::http::request&& request, std::function<void(boost::asynchronous::http::status, boost::asynchronous::http::request&&)> callback);

    boost::asynchronous::http::socket m_socket;
    boost::asio::ip::tcp::endpoint m_endpoint;
    std::shared_ptr<boost::asio::streambuf> m_retained;
};

class connection_proxy : public boost::asynchronous::servant_proxy<boost::asynchronous::http::connection_proxy, boost::asynchronous::http::connection>
{
public:
    connection_proxy(boost::asynchronous::any_shared_scheduler_proxy<> scheduler, boost::asynchronous::http::socket&& socket);

    BOOST_ASYNC_POST_MEMBER_LOG(start, "connection_proxy::start", BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY)
    BOOST_ASYNC_POST_MEMBER_LOG(send, "connection_proxy::send", BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY)
    BOOST_ASYNC_POST_MEMBER_LOG(read_body, "connection_proxy::read_body", BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY)
};

class server : public boost::asynchronous::http::trackable_servant
{
public:
    using route = std::function<void(std::shared_ptr<boost::asynchronous::http::connection_proxy>, boost::asynchronous::http::request&&, std::function<void(boost::asynchronous::http::response&&)>)>;

    server(boost::asynchronous::http::weak_scheduler scheduler, boost::asynchronous::any_shared_scheduler_proxy<> asio_scheduler, const std::string& address, unsigned int port, const boost::asynchronous::http::security& security);
    
    void add_route(std::vector<std::string>&& methods, boost::regex&& uri, route&& route);
    void set_error_page(boost::asynchronous::http::status status, boost::asynchronous::http::content&& content);
    
private:
    void handle_connect(boost::asynchronous::http::socket&& socket);
    void handle_request(std::weak_ptr<boost::asynchronous::http::connection_proxy> weak_connection, boost::asynchronous::http::status status, boost::asynchronous::http::request&& request);
    void handle_completed(std::weak_ptr<boost::asynchronous::http::connection_proxy> weak_connection);    
    void send_final_response(std::shared_ptr<boost::asynchronous::http::connection_proxy> connection, boost::asynchronous::http::response&& response);

    std::string m_address;
    unsigned int m_port;
    boost::asynchronous::any_shared_scheduler_proxy<> m_asio_scheduler;
    boost::asynchronous::http::acceptor_proxy m_acceptor_proxy;
    std::set<std::shared_ptr<boost::asynchronous::http::connection_proxy>> m_pending;
    
    std::vector<std::tuple<boost::regex, std::vector<std::string>, route>> m_routes;
    std::map<boost::asynchronous::http::status, boost::asynchronous::http::content> m_error_pages;
};

class server_proxy : public boost::asynchronous::http::servant_proxy<boost::asynchronous::http::server_proxy, boost::asynchronous::http::server>
{
public:
    server_proxy(boost::asynchronous::http::shared_scheduler scheduler, boost::asynchronous::any_shared_scheduler_proxy<> asioScheduler, const std::string& address, unsigned int port, const boost::asynchronous::http::security& security);
    
    // Forward to the templated version (these versions are there to enforce type constraints and enable type conversions to boost::regex)
    void add_route(std::vector<std::string> methods, boost::regex uri, boost::asynchronous::http::server::route route);
    void add_route(std::vector<std::string> methods, std::string uri, boost::asynchronous::http::server::route route);
    
    BOOST_ASYNC_POST_MEMBER_LOG(set_error_page, "boost::asynchronous::http::server_proxy::set_error_page", BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY)

private:
    BOOST_ASYNC_POST_MEMBER_LOG(add_route, "boost::asynchronous::http::server_proxy::add_route", BOOST_ASYNCHRONOUS_HTTP_TASK_PRIORITY)
};

}
}
}
