#ifndef BOOST_ASYNCHRONOUS_BIN_ARCHIVE_TYPES_HPP
#define BOOST_ASYNCHRONOUS_BIN_ARCHIVE_TYPES_HPP

#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/type_erasure/callable.hpp>
#include <boost/asynchronous/job_traits.hpp>
#include <boost/asynchronous/scheduler/tcp/simple_tcp_client.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/job_server.hpp>
#include <portable_binary_oarchive.hpp>
#include <portable_binary_iarchive.hpp>

namespace boost { namespace asynchronous
{
typedef
boost::type_erasure::any<
    boost::mpl::vector<
        boost::asynchronous::any_callable_concept,
        boost::asynchronous::has_save<void(portable_binary_oarchive&,const unsigned int)>,
        boost::asynchronous::has_load<void(portable_binary_iarchive&,const unsigned int)>,
        boost::asynchronous::has_get_task_name<std::string()>
    >,
    boost::type_erasure::_self
> any_bin_serializable_helper;

struct any_bin_serializable: public boost::asynchronous::any_bin_serializable_helper
{
    any_bin_serializable(){}
    template <class T>
    any_bin_serializable(T t):any_bin_serializable_helper(t){}
    typedef portable_binary_oarchive oarchive;
    typedef portable_binary_iarchive iarchive;
};

template< >
struct job_traits< boost::asynchronous::any_bin_serializable >
{
    typedef typename boost::asynchronous::default_loggable_job<
                                  boost::chrono::high_resolution_clock >            diagnostic_type;
    typedef boost::asynchronous::detail::serializable_base_job<
            diagnostic_type,boost::asynchronous::any_bin_serializable >                     wrapper_type;

    typedef typename diagnostic_type::diagnostic_item_type                          diagnostic_item_type;
    typedef boost::asynchronous::diagnostics_table<
            std::string,diagnostic_item_type>                                       diagnostic_table_type;

    static bool get_failed(boost::asynchronous::any_bin_serializable const& )
    {
        return false;
    }
    static void set_posted_time(boost::asynchronous::any_bin_serializable& )
    {
    }
    static void set_started_time(boost::asynchronous::any_bin_serializable& )
    {
    }
    static void set_finished_time(boost::asynchronous::any_bin_serializable& )
    {
    }
    static void set_name(boost::asynchronous::any_bin_serializable& , std::string const& )
    {
    }
    static std::string get_name(boost::asynchronous::any_bin_serializable& )
    {
      return "";
    }
    static diagnostic_item_type get_diagnostic_item(boost::asynchronous::any_bin_serializable& )
    {
      return diagnostic_item_type();
    }
    static void set_interrupted(boost::asynchronous::any_bin_serializable& , bool )
    {
    }
    template <class Diag>
    static void add_diagnostic(boost::asynchronous::any_bin_serializable& ,Diag* )
    {
    }
    static void set_failed(boost::asynchronous::any_bin_serializable& )
    {
    }
};
namespace tcp
{
// the proxy of AsioCommunicationServant for use in an external thread
class simple_bin_tcp_client_proxy: public boost::asynchronous::servant_proxy<simple_bin_tcp_client_proxy,
                               boost::asynchronous::tcp::simple_tcp_client<
                                boost::asynchronous::tcp::client_time_check_policy<boost::asynchronous::any_bin_serializable>,
                                boost::asynchronous::any_bin_serializable> >
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template <class Scheduler,typename... Args>
    simple_bin_tcp_client_proxy(Scheduler s,
                            boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_bin_serializable> pool,
                            const std::string& server, const std::string& path,
                            std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                               std::function<void(boost::asynchronous::tcp::client_request const&)>)> const& executor,
                            Args... args):
        boost::asynchronous::servant_proxy<simple_bin_tcp_client_proxy,
        boost::asynchronous::tcp::simple_tcp_client<boost::asynchronous::tcp::client_time_check_policy<boost::asynchronous::any_bin_serializable>,
                                                    boost::asynchronous::any_bin_serializable> >
            (s,pool,server,path,executor,args...)
    {}
    // we offer a single member for posting
    BOOST_ASYNC_FUTURE_MEMBER(run)
};
class simple_bin_tcp_client_proxy_queue_size:
        public boost::asynchronous::servant_proxy<simple_bin_tcp_client_proxy_queue_size,
                                          boost::asynchronous::tcp::simple_tcp_client<
                                            boost::asynchronous::tcp::queue_size_check_policy<
                                                boost::asynchronous::any_bin_serializable>,
                                                boost::asynchronous::any_bin_serializable > >
{
public:
    // ctor arguments are forwarded to AsioCommunicationServant
    template <class Scheduler,typename... Args>
    simple_bin_tcp_client_proxy_queue_size(Scheduler s,
                                       boost::asynchronous::any_shared_scheduler_proxy<boost::asynchronous::any_bin_serializable> pool,
                                       const std::string& server, const std::string& path,
                                       std::function<void(std::string const&,boost::asynchronous::tcp::server_reponse,
                                               std::function<void(boost::asynchronous::tcp::client_request const&)>)> const& executor,
                                       Args... args):
        boost::asynchronous::servant_proxy<simple_bin_tcp_client_proxy_queue_size,
        boost::asynchronous::tcp::simple_tcp_client<boost::asynchronous::tcp::queue_size_check_policy<boost::asynchronous::any_bin_serializable>,
                                                    boost::asynchronous::any_bin_serializable > >
            (s,pool,server,path,executor,args...)
    {}
    // we offer a single member for posting
    BOOST_ASYNC_FUTURE_MEMBER(run)
};

// choose correct proxy

template <>
struct get_correct_simple_tcp_client_proxy<
        boost::asynchronous::tcp::queue_size_check_policy<boost::asynchronous::any_bin_serializable>,
        boost::asynchronous::any_bin_serializable>
{
    typedef boost::asynchronous::tcp::simple_bin_tcp_client_proxy_queue_size type;
};
template <>
struct get_correct_simple_tcp_client_proxy<
        boost::asynchronous::tcp::client_time_check_policy<boost::asynchronous::any_bin_serializable>,
        boost::asynchronous::any_bin_serializable>
{
    typedef boost::asynchronous::tcp::simple_bin_tcp_client_proxy type;
};

class job_server_proxy_bin_serializable :
        public boost::asynchronous::servant_proxy<job_server_proxy_bin_serializable,
                    boost::asynchronous::tcp::job_server<boost::asynchronous::any_callable,boost::asynchronous::any_bin_serializable> >
{
public:

    typedef job_server<boost::asynchronous::any_callable,boost::asynchronous::any_bin_serializable> servant_type;
    template <class Scheduler,class Worker>
    job_server_proxy_bin_serializable(Scheduler s, Worker w,std::string const & address,unsigned int port, bool stealing):
        boost::asynchronous::servant_proxy<job_server_proxy_bin_serializable,
                                           job_server<boost::asynchronous::any_callable,boost::asynchronous::any_bin_serializable> >
                    (s,w,address,port,stealing)
    {}

    // get_data is posted, no future, no callback
    BOOST_ASYNC_FUTURE_MEMBER(add_task)
    BOOST_ASYNC_FUTURE_MEMBER(requests_stealing)
    BOOST_ASYNC_FUTURE_MEMBER(no_jobs)

};
template <>
struct get_correct_job_server_proxy<boost::asynchronous::any_callable,boost::asynchronous::any_bin_serializable>
{
    typedef boost::asynchronous::tcp::job_server_proxy_bin_serializable type;
};
} //namespace tcp
}}

#endif // BOOST_ASYNCHRONOUS_BIN_ARCHIVE_TYPES_HPP
