// Boost.Asynchronous library
//  Copyright (C) Daniel Schmitt 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_PARTITION_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_PARTITION_HPP

#include <atomic>
#include <list>
#include <memory>
#include <utility>
#include <algorithm>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>
#include <type_traits>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <class It, class Func>
void median_of_three(It it1, It it2, It it3, Func& func)
{
    if (func(*it2,*it1))
    {
        std::iter_swap(it1,it2);
    }
    if (func(*it3,*it1))
    {
        std::iter_swap(it1,it3);
    }
    if (func(*it3,*it2))
    {
        std::iter_swap(it2,it3);
    }
}
template <class It, class Func>
auto median_of_medians(It beg, It end, Func& func)
-> typename std::remove_reference<decltype(*beg)>::type
{
    auto dist = std::distance(beg,end);
    if (dist < 3)
        return *beg;
    if (dist < 16)
    {
        std::size_t offset = dist/2;
        std::vector<typename std::remove_reference<decltype(*beg)>::type> temp1 = {*beg, *(beg+offset), *(end-1)};
        boost::asynchronous::detail::median_of_three(temp1.begin(),temp1.begin()+1,temp1.begin()+2,func);
        return *(temp1.begin()+1);
    }

    std::size_t offset = dist/8;
    std::vector<typename std::remove_reference<decltype(*beg)>::type> temp1 = {*beg, *(beg+offset), *(beg+offset*2)};
    boost::asynchronous::detail::median_of_three(temp1.begin(),temp1.begin()+1,temp1.begin()+2,func);

    std::vector<typename std::remove_reference<decltype(*beg)>::type> temp2 = {*(beg+ 3*offset), *(beg+offset*4), *(end-(3*offset+1))};
    boost::asynchronous::detail::median_of_three(temp2.begin(),temp2.begin()+1,temp2.begin()+2,func);

    std::vector<typename std::remove_reference<decltype(*beg)>::type> temp3 = {*(end-(2*offset+1)), *(end-(offset+1)), *(end-1)};
    boost::asynchronous::detail::median_of_three(temp3.begin(),temp3.begin()+1,temp3.begin()+2,func);

    std::vector<typename std::remove_reference<decltype(*beg)>::type> res = {*(temp1.begin()+1), *(temp2.begin()+1), *(temp3.begin()+1)};
    boost::asynchronous::detail::median_of_three(res.begin(),res.begin()+1,res.begin()+2,func);

    return *(res.begin()+1);
}

template <class iterator, class relation, class shared_data, class JobType>
struct partition_worker : public boost::asynchronous::continuation_task<std::pair<std::list<std::pair<iterator, iterator>>, std::list<std::pair<iterator, iterator>>>>
{

    partition_worker(shared_data* sd)
        : boost::asynchronous::continuation_task<std::pair<std::list<std::pair<iterator, iterator>>, std::list<std::pair<iterator, iterator>>>>("partition_worker"), m_sd(sd) { }

    void operator()()
    {
        boost::asynchronous::continuation_result<std::pair<std::list<std::pair<iterator, iterator>>, std::list<std::pair<iterator, iterator>>>> task_res = this->this_task_result();
        try
        {
            int threads_left = m_sd->m_open_threads.fetch_sub(1);
            if (threads_left < 1 || m_sd->m_size < 0)
            {
                m_sd->m_open_threads.fetch_add(1);
                auto L = part_parallel();
                task_res.set_value(std::move(L));
                return;
            }
            else
            {
                auto sd = m_sd;
                boost::asynchronous::create_callback_continuation_job<JobType>(
                [task_res, sd]
                (std::tuple<boost::asynchronous::expected<std::pair<std::list<std::pair<iterator, iterator>>, std::list<std::pair<iterator, iterator>>>>,boost::asynchronous::expected<std::pair<std::list<std::pair<iterator, iterator>>, std::list<std::pair<iterator, iterator>>>> > res)
                mutable
                {
                    try
                    {
                        //MERGE
                        std::pair<std::list<std::pair<iterator, iterator>>, std::list<std::pair<iterator, iterator>>> LR = std::move(std::get<0>(res).get());
                        std::list<std::pair<iterator, iterator>> l_rest = std::move(LR.first);
                        std::list<std::pair<iterator, iterator>> r_rest = std::move(LR.second);
                        std::pair<std::list<std::pair<iterator, iterator>>, std::list<std::pair<iterator, iterator>>> RR = std::move(std::get<1>(res).get());
                        l_rest.splice(l_rest.end(), RR.first);
                        r_rest.splice(r_rest.end(), RR.second);

                        iterator lbs = sd->m_left, rbs = sd->m_right, lbe = sd->m_left, rbe = sd->m_right;
                        while((!r_rest.empty() || rbs != rbe) && (!l_rest.empty() || lbs != lbe))
                        {
                            if (rbs == rbe)
                            {
                                rbs = r_rest.front().first;
                                rbe = r_rest.front().second;
                                r_rest.pop_front();
                            }
                            if (lbs == lbe)
                            {
                                lbs = l_rest.front().first;
                                lbe = l_rest.front().second;
                                l_rest.pop_front();
                            }

                            for(;;)
                            {
                                //while (lbs < lbe && sd->m_r(*(lbs))) ++lbs;
                                //while (rbs < rbe && !sd->m_r(*(rbs))) ++rbs;
                                for(auto i = lbs; i != lbe;++i)
                                {
                                    if (!sd->m_r(*(i)))
                                        break;
                                    ++lbs;

                                }
                                for(auto i = rbs; i != rbe;++i)
                                {
                                    if (sd->m_r(*(i)))
                                        break;
                                    ++rbs;
                                }
                                if (lbs == lbe || rbs == rbe) break;
                                std::swap(*lbs,*rbs);
                            }
                        }
                        if (lbs != lbe)
                            l_rest.push_back(std::make_pair(lbs,lbe));
                        if (rbs != rbe)
                            r_rest.push_back(std::make_pair(rbs,rbe));
                        task_res.set_value(std::make_pair(std::move(l_rest), std::move(r_rest)));
                        return;
                    }
                    catch (std::exception& e)
                    {
                        task_res.set_exception(boost::copy_exception(e));
                        return;
                    }
                },
                // future results of recursive tasks
                partition_worker<iterator, relation, shared_data, JobType>(m_sd),
                partition_worker<iterator, relation, shared_data, JobType>(m_sd)
                );
            }
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }

private:

    shared_data* m_sd;

    std::pair<std::list<std::pair<iterator, iterator>>, std::list<std::pair<iterator, iterator>>> part_parallel()
    {
        iterator lbs = m_sd->m_left, rbs = m_sd->m_right, lbe = m_sd->m_left, rbe = m_sd->m_right;
        int mlstart = 0, mrstart = 0;
        int myrtrue = 0;

        while(m_sd->m_size > 0)
        {
            int mysize = m_sd->m_size.fetch_sub(m_sd->m_BS);
            if (mysize <= 0) break;
            if (mysize > m_sd->m_BS) mysize = m_sd->m_BS;

            if (lbs == lbe)
            {
                mlstart = m_sd->m_lmove.fetch_add(mysize);
                lbs = m_sd->m_left + mlstart;
                lbe = lbs + mysize;
            }
            else
                //if (rbs == rbe)
            {
                mrstart = m_sd->m_rmove.fetch_sub(mysize);
                rbs = m_sd->m_left + mrstart;
                rbe = rbs - mysize;
                --rbs;
                --rbe;
            }

            for(;;)
            {
                //while (lbs < lbe && m_sd->m_r(*(lbs))) {++myrtrue; ++lbs;}
                //while (rbs > rbe && !m_sd->m_r(*(rbs))) --rbs;
                for(auto i = lbs; i != lbe;++i)
                {
                    if (!m_sd->m_r(*(i)))
                        break;
                    ++myrtrue;
                    ++lbs;
                }
                for(auto i = rbs; i != rbe;--i)
                {
                    if (m_sd->m_r(*(i)))
                        break;
                    --rbs;
                }
                if (lbs == lbe || rbs == rbe) break;
                std::swap(*lbs,*rbs);
            }
        }

        std::list<std::pair<iterator, iterator>> L,R;
        if (lbs != lbe)
        {
            for(auto i = lbs; i < lbe ; ++i) if(m_sd->m_r(*(i))) ++myrtrue;
            L.push_back(std::make_pair(lbs,lbe));
        }
        if (rbs != rbe)
        {
            ++rbe;
            ++rbs;
            for(auto i = rbe; i < rbs ; ++i) if(m_sd->m_r(*(i))) ++myrtrue;
            R.push_back(std::make_pair(rbe,rbs));
        }
        m_sd->m_rtrue += myrtrue;

        return std::make_pair(std::move(L), std::move(R));
    }
};

template <class Iterator, class relation, class JobType>
struct parallel_partition_helper : public boost::asynchronous::continuation_task<Iterator>
{
    struct shared_data
    {
        shared_data(const Iterator left, const Iterator right, relation r, const uint32_t open_threads)
            : m_r(std::move(r)), m_BS(std::max((int)std::distance(left,right) / 1000, (int)1000)), m_left(left), m_right(right)
            , m_open_threads(open_threads), m_size(right-left), m_lmove(0), m_rmove(right-left), m_rtrue(0)
        {
            if (m_size < 10000)
                m_open_threads = 1;
            if (m_open_threads < 1)
                m_open_threads = 1;
            if (m_open_threads > m_size)
                m_open_threads = 1;
        }
        shared_data(const Iterator left, const Iterator right, relation r, const unsigned BS, const uint32_t open_threads)
            : m_r(std::move(r)), m_BS(BS), m_left(left), m_right(right)
            , m_open_threads(open_threads), m_size(right-left), m_lmove(0), m_rmove(right-left), m_rtrue(0)
        {
            if (m_size < 10000) m_open_threads = 1;
            if (m_open_threads < 1) m_open_threads = 1;
            if (m_open_threads > m_size) m_open_threads = 1;
        }

        const relation m_r;
        const int m_BS;
        const Iterator m_left, m_right;
        std::atomic<uint32_t> m_open_threads;
        std::atomic<int64_t> m_size, m_lmove, m_rmove, m_rtrue;
    };

    parallel_partition_helper(const Iterator a, const Iterator b, relation r, const uint32_t thread_num,const std::string& task_name)
        : boost::asynchronous::continuation_task<Iterator>(task_name)
        , m_sd(std::make_shared<shared_data>(a, b, std::move(r), thread_num))
    { }

    void operator()()
    {
        boost::asynchronous::continuation_result<Iterator> task_res = this->this_task_result();
        try
        {
            if (m_sd->m_right - m_sd->m_left == 0)
            {
                task_res.set_value(m_sd->m_right);
                return;
            }

            auto c = boost::asynchronous::top_level_callback_continuation_job<std::pair<std::list<std::pair<Iterator, Iterator>>, std::list<std::pair<Iterator, Iterator>>>, JobType>
                    (boost::asynchronous::detail::partition_worker<Iterator, relation, shared_data, JobType>(m_sd.get()));

            shared_data* sd= m_sd.get();
            auto msd = m_sd;

            c.on_done(
               [task_res, msd,sd]
               (std::tuple<boost::asynchronous::expected<std::pair<std::list<std::pair<Iterator, Iterator>>, std::list<std::pair<Iterator, Iterator>>>> > res)
                mutable
            {

                std::pair<std::list<std::pair<Iterator, Iterator>>, std::list<std::pair<Iterator, Iterator>>> R = std::move(std::get<0>(res).get());
                std::list<std::pair<Iterator, Iterator>> l_rest = std::move(R.first);
                std::list<std::pair<Iterator, Iterator>> r_rest = std::move(R.second);

                auto it_left_plus_rtrue = sd->m_left;
                auto it_left_plus_lmove = sd->m_left;
                std::advance(it_left_plus_rtrue,sd->m_rtrue.load());
                std::advance(it_left_plus_lmove,sd->m_lmove.load());

                if (sd->m_lmove < sd->m_rtrue)
                {
                    for(auto& r : r_rest)
                    {
                        if(r.first < it_left_plus_rtrue) r.first = it_left_plus_rtrue;
                        if(r.second < it_left_plus_rtrue) r.second = it_left_plus_rtrue;
                    }
                    l_rest.push_back(std::make_pair(it_left_plus_lmove,it_left_plus_rtrue));
                }
                if (sd->m_lmove > sd->m_rtrue)
                {
                    for(auto& l : l_rest)
                    {
                        if(l.first > it_left_plus_rtrue) l.first = it_left_plus_rtrue;
                        if(l.second > it_left_plus_rtrue) l.second = it_left_plus_rtrue;
                    }
                    r_rest.push_back(std::make_pair(it_left_plus_rtrue,it_left_plus_lmove));
                }

                Iterator lbs = sd->m_left, rbs = sd->m_right, lbe = sd->m_left, rbe = sd->m_right;
                while((!r_rest.empty() || rbs != rbe) && (!l_rest.empty() || lbs != lbe))
                {
                    if (rbs == rbe)
                    {
                        rbs = r_rest.front().first;
                        rbe = r_rest.front().second;
                        r_rest.pop_front();
                    }
                    if (lbs == lbe)
                    {
                        lbs = l_rest.front().first;
                        lbe = l_rest.front().second;
                        l_rest.pop_front();
                    }

                    for(;;)
                    {
                        while (lbs < lbe && sd->m_r(*(lbs))) ++lbs;
                        while (rbs < rbe && !sd->m_r(*(rbs))) ++rbs;
                        if (lbs == lbe || rbs == rbe) break;
                        std::swap(*lbs,*rbs);
                    }
                }

                task_res.set_value(it_left_plus_rtrue);
                return;
            });
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
private:
    std::shared_ptr<shared_data> m_sd;
};
}

template <class Iterator,class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator,Job>
parallel_partition(Iterator beg, Iterator end, Func func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const uint32_t thread_num,const std::string& task_name, std::size_t /*prio*/=0)
#else
                   const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string& task_name="", std::size_t /*prio*/=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator,Job>
            (boost::asynchronous::detail::parallel_partition_helper<Iterator,Func,Job>
             (beg,end,std::move(func),thread_num,task_name));

}

// version for moved ranges => will return the range as continuation
template <class Range, class Iterator,class Func, class Job,class Enable=void>
struct parallel_partition_range_move_helper:
        public boost::asynchronous::continuation_task<std::pair<Range,Iterator>>
{
    parallel_partition_range_move_helper(std::shared_ptr<Range> range,Iterator beg, Iterator end,Func func,
                                         const uint32_t thread_num,
                                         const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<std::pair<Range,Iterator>>(task_name)
        ,range_(range),beg_(beg),end_(end),func_(std::move(func)),thread_num_(thread_num),prio_(prio)
    {
    }
    parallel_partition_range_move_helper(parallel_partition_range_move_helper&&)=default;
    parallel_partition_range_move_helper& operator=(parallel_partition_range_move_helper&&)=default;
    parallel_partition_range_move_helper(parallel_partition_range_move_helper const&)=delete;
    parallel_partition_range_move_helper& operator=(parallel_partition_range_move_helper const&)=delete;

    void operator()()
    {
        boost::asynchronous::continuation_result<std::pair<Range,Iterator>> task_res = this->this_task_result();
        try
        {
            std::shared_ptr<Range> range = range_;
            // TODO better ctor?
            auto cont = boost::asynchronous::parallel_partition<decltype(beg_),Func,Job>
                    (beg_,end_,std::move(func_),thread_num_,this->get_name(),prio_);
            cont.on_done([task_res,range]
                          (std::tuple<boost::asynchronous::expected<Iterator> >&& continuation_res) mutable
            {
                try
                {
                    task_res.set_value(std::make_pair(std::move(*range),std::get<0>(continuation_res).get()));
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            });
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    std::shared_ptr<Range> range_;
    Iterator beg_;
    Iterator end_;
    Func func_;
    uint32_t thread_num_;
    std::size_t prio_;
};


template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
auto parallel_partition(Range&& range,Func func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                        const uint32_t thread_num,const std::string& task_name, std::size_t prio=0)
#else
                        const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string& task_name="", std::size_t prio=0)
#endif
-> typename std::enable_if<
              !boost::asynchronous::detail::has_is_continuation_task<Range>::value,
              // TODO make it work with boost::begin and clang
              boost::asynchronous::detail::callback_continuation<std::pair<Range,decltype(range.begin())>,Job> >::type

{
    auto r = std::make_shared<Range>(std::forward<Range>(range));
    auto beg = boost::begin(*r);
    auto end = boost::end(*r);
    return boost::asynchronous::top_level_callback_continuation_job<std::pair<Range,decltype(boost::begin(range))>,Job>
            (boost::asynchronous::parallel_partition_range_move_helper<Range,decltype(beg),Func,Job>
                (r,beg,end,func,thread_num,task_name,prio));
}

namespace detail
{
// Continuation is a callback continuation
template <class Continuation, class Iterator,class Func, class Job>
struct parallel_partition_continuation_helper:
        public boost::asynchronous::continuation_task<
            std::pair<
                typename Continuation::return_type,
                Iterator>
       >
{
    parallel_partition_continuation_helper(Continuation const& c,Func func, const uint32_t thread_num,
                        const std::string& task_name, std::size_t prio)
        :boost::asynchronous::continuation_task<
            std::pair<
                typename Continuation::return_type,
                Iterator>
          >(task_name)
        ,cont_(c),func_(std::move(func)),thread_num_(thread_num),prio_(prio)
    {}
    void operator()()
    {
        using return_type=
        std::pair<
            typename Continuation::return_type,
            Iterator>;

        boost::asynchronous::continuation_result<return_type> task_res = this->this_task_result();
        try
        {
            auto thread_num = thread_num_;
            auto task_name = this->get_name();
            auto prio = prio_;
            auto func= std::move(func_);
            cont_.on_done([task_res,func,thread_num,task_name,prio]
                          (std::tuple<boost::asynchronous::expected<typename Continuation::return_type> >&& continuation_res) mutable
            {
                try
                {
                    auto new_continuation =
                       boost::asynchronous::parallel_partition<typename Continuation::return_type, Func, Job>
                            (std::move(std::get<0>(continuation_res).get()),std::move(func),thread_num,task_name,prio);
                    new_continuation.on_done(
                    [task_res]
                    (std::tuple<boost::asynchronous::expected<return_type> >&& new_continuation_res)
                    {
                        task_res.set_value(std::move(std::get<0>(new_continuation_res).get()));
                    });
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            }
            );
        }
        catch(std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }
    Continuation cont_;
    Func func_;
    uint32_t thread_num_;
    std::size_t prio_;
};
}

template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<
        boost::asynchronous::detail::has_is_continuation_task<Range>::value,
        boost::asynchronous::detail::callback_continuation<
              std::pair<typename Range::return_type,decltype(boost::begin(std::declval<typename Range::return_type&>()))>,
              Job>
>::type
parallel_partition(Range&& range,Func func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   const uint32_t thread_num,const std::string& task_name, std::size_t prio=0)
#else
                   const uint32_t thread_num = boost::thread::hardware_concurrency(),const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<
                std::pair<typename Range::return_type,decltype(boost::begin(std::declval<typename Range::return_type&>()))>,
                Job
            >
            (boost::asynchronous::detail::parallel_partition_continuation_helper<
                Range,
                decltype(boost::begin(std::declval<typename Range::return_type&>())),
                Func,
                Job>
                    (std::forward<Range>(range),func,thread_num,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_PARTITION_HPP
