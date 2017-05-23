// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP
#define BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP

#include <boost/geometry/algorithms/detail/partition.hpp>

#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>

namespace boost { namespace geometry
{

namespace detail { namespace partition
{
// Divide collection into three subsets: lower, upper and oversized
// (not-fitting)
// (lower == left or bottom, upper == right or top)
template <typename OverlapsPolicy, typename InputCollection, typename Box,typename Iterator>
inline void divide_into_subsets_it(Box const& lower_box,
        Box const& upper_box,
        InputCollection const& collection,
        Iterator beg,
        Iterator end,
        index_vector_type& lower,
        index_vector_type& upper,
        index_vector_type& exceeding)
{
    typedef boost::range_iterator
        <
            index_vector_type const
        >::type index_iterator_type;

    for(auto it = beg;it != end;++it)
    {
        bool const lower_overlapping = OverlapsPolicy::apply(lower_box,
                    collection[*it]);
        bool const upper_overlapping = OverlapsPolicy::apply(upper_box,
                    collection[*it]);

        if (lower_overlapping && upper_overlapping)
        {
            exceeding.push_back(*it);
        }
        else if (lower_overlapping)
        {
            lower.push_back(*it);
        }
        else if (upper_overlapping)
        {
            upper.push_back(*it);
        }
        else
        {
            // Is nowhere! Should not occur!
            //BOOST_ASSERT(false);
        }
    }
}


template
<
    int Dimension,
    typename Box,
    typename OverlapsPolicy1,
    typename OverlapsPolicy2,
    typename ExpandPolicy1,
    typename ExpandPolicy2,
    typename VisitBoxPolicy,
    typename Job
>
class parallel_partition_two_collections
{
    typedef parallel_partition_two_collections this_type;

    typedef std::vector<std::size_t> index_vector_type;
    //typedef typename coordinate_type<Box>::type ctype;
    typedef parallel_partition_two_collections
            <
                1 - Dimension,
                Box,
                OverlapsPolicy1,
                OverlapsPolicy2,
                ExpandPolicy1,
                ExpandPolicy2,
                VisitBoxPolicy,
                Job
            > sub_divide;

    typedef boost::range_iterator
        <
            index_vector_type const
        >::type index_iterator_type;

    template<int, class,class,class,class,class,class,class> friend class parallel_partition_two_collections;


    template
    <
        typename InputCollection1,
        typename InputCollection2,
        typename Policy
    >
    static inline void next_level(Box const& box,
            InputCollection1 const& collection1,
            index_vector_type const& input1,
            InputCollection2 const& collection2,
            index_vector_type const& input2,
            int level, std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {
        partition_two_collections
        <
            1 - Dimension,
            Box,
            OverlapsPolicy1,
            OverlapsPolicy2,
            ExpandPolicy1,
            ExpandPolicy2,
            VisitBoxPolicy
        >::apply(box, collection1, input1, collection2, input2,
                level + 1, min_elements,
                policy, box_policy);
    }

    template
    <
        typename ExpandPolicy,
        typename InputCollection
    >
    static inline Box get_new_box(InputCollection const& collection,
                    index_vector_type const& input)
    {
        Box box;
        geometry::assign_inverse(box);
        expand_with_elements<ExpandPolicy>(box, collection, input);
        return box;
    }

    template
    <
        typename InputCollection1,
        typename InputCollection2
    >
    static inline Box get_new_box(InputCollection1 const& collection1,
                    index_vector_type const& input1,
                    InputCollection2 const& collection2,
                    index_vector_type const& input2)
    {
        Box box = get_new_box<ExpandPolicy1>(collection1, input1);
        expand_with_elements<ExpandPolicy2>(box, collection2, input2);
        return box;
    }

    template
    <
        typename InputCollection1,
        typename InputCollection2,
        typename Policy,
        typename Owner
    >
    struct simple_parallel_partition_task : public boost::asynchronous::continuation_task<Policy>
    {
        simple_parallel_partition_task(Box const& box,
                                std::shared_ptr<InputCollection1> collection1,std::shared_ptr<index_vector_type> input1,
                                index_iterator_type beg1,index_iterator_type end1,
                                std::shared_ptr<InputCollection2> collection2,std::shared_ptr<index_vector_type> input2,
                                index_iterator_type beg2,index_iterator_type end2,
                                int level,
                                std::size_t min_elements,
                                std::shared_ptr<Policy> policy, VisitBoxPolicy& box_policy,
                                long cutoff,const std::string& task_name, std::size_t prio)
            : boost::asynchronous::continuation_task<Policy>(task_name)
            // TODO move
            , box_(box)
            , collection1_(collection1), collection2_(collection2)
            , input1_(input1), beg1_(beg1), end1_(end1)
            , input2_(input2), beg2_(beg2), end2_(end2)
            , level_(level)
            , min_elements_(min_elements),policy_(policy),box_policy_(box_policy)
            , cutoff_(cutoff), prio_(prio)
        {}

        void operator()()
        {
            boost::asynchronous::continuation_result<Policy> task_res = this->this_task_result();
            auto d1 = std::distance(beg1_,end1_);
            auto d2 = std::distance(beg2_,end2_);
            if (d1 > d2)
            {
                // advance up to cutoff
                index_iterator_type it1 = boost::asynchronous::detail::find_cutoff(beg1_,cutoff_,end1_);
                // if not at end, recurse, otherwise execute here
                if (it1 == end1_)
                {
                    auto policy = std::make_shared<Policy>(std::move(policy_->clone()));
                    Owner::apply3(box_,collection1_,beg1_,end1_,collection2_,beg2_,end2_,level_,min_elements_,policy,box_policy_);
                    task_res.set_value(std::move(*policy));
                }
                else
                {
                    auto level = level_;
                    boost::asynchronous::create_callback_continuation_job<Job>(
                                // called when subtasks are done, set our result
                                [task_res,level]
                                (std::tuple<boost::asynchronous::expected<Policy>,
                                            boost::asynchronous::expected<Policy> >&& res) mutable
                                {
                                    try
                                    {
                                        auto nl1 = std::move(std::get<0>(res).get());
                                        auto nl2 = std::move(std::get<1>(res).get());
                                        (nl1).merge(nl2);
                                        task_res.set_value(std::move(nl1));
                                    }
                                    catch(std::exception& e)
                                    {
                                        task_res.set_exception(std::make_exception_ptr(e));
                                    }
                                },
                                // recursive tasks
                                simple_parallel_partition_task<InputCollection1,InputCollection2,Policy,this_type>
                                    (box_,
                                     collection1_,input1_,beg1_,it1,
                                     collection2_,input2_,beg2_,end2_,
                                     level_+1,min_elements_,policy_,box_policy_,cutoff_,this->get_name(),prio_),
                                simple_parallel_partition_task<InputCollection1,InputCollection2,Policy,this_type>
                                    (box_,
                                     collection1_,input1_,it1,end1_,
                                     collection2_,input2_,beg2_,end2_,
                                     level_+1,min_elements_,policy_,box_policy_,cutoff_,this->get_name(),prio_)
                       );
                }
            }
            else
            {
                // advance up to cutoff
                index_iterator_type it2 = boost::asynchronous::detail::find_cutoff(beg2_,cutoff_,end2_);
                // if not at end, recurse, otherwise execute here
                if (it2 == end2_)
                {
                    auto policy = std::make_shared<Policy>(std::move(policy_->clone()));

                    Owner::apply3(box_,collection1_,beg1_,end1_,collection2_,beg2_,end2_,level_,min_elements_,policy,box_policy_);
                    task_res.set_value(std::move(*policy));
                }
                else
                {
                    auto level = level_;
                    boost::asynchronous::create_callback_continuation_job<Job>(
                                // called when subtasks are done, set our result
                                [task_res,level]
                                (std::tuple<boost::asynchronous::expected<Policy>,
                                            boost::asynchronous::expected<Policy> >&& res) mutable
                                {
                                    try
                                    {
                                        auto nl1 = std::move(std::get<0>(res).get());
                                        auto nl2 = std::move(std::get<1>(res).get());
                                        (nl1).merge(nl2);
                                        task_res.set_value(std::move(nl1));
                                    }
                                    catch(std::exception& e)
                                    {
                                        task_res.set_exception(std::make_exception_ptr(e));
                                    }
                                },
                                // recursive tasks
                                simple_parallel_partition_task<InputCollection1,InputCollection2,Policy,this_type>
                                    (box_,
                                     collection1_,input1_,beg1_,end1_,
                                     collection2_,input2_,beg2_,it2,
                                     level_+1,min_elements_,policy_,box_policy_,cutoff_,this->get_name(),prio_),
                                simple_parallel_partition_task<InputCollection1,InputCollection2,Policy,this_type>
                                    (box_,
                                     collection1_,input1_,beg1_,end1_,
                                     collection2_,input2_,it2,end2_,
                                     level_+1,min_elements_,policy_,box_policy_,cutoff_,this->get_name(),prio_)
                       );
                }
            }
        }

        Box box_;
        std::shared_ptr<InputCollection1> collection1_;
        std::shared_ptr<InputCollection2> collection2_;
        std::shared_ptr<index_vector_type> input1_;
        index_iterator_type beg1_;
        index_iterator_type end1_;
        std::shared_ptr<index_vector_type> input2_;
        index_iterator_type beg2_;
        index_iterator_type end2_;
        int level_;
        std::size_t min_elements_;
        std::shared_ptr<Policy> policy_;
        VisitBoxPolicy box_policy_;
        long cutoff_;
        std::size_t prio_;
    };
public :
    template
    <
        typename InputCollection1,
        typename InputCollection2,
        typename Policy
    >
    static inline
    boost::asynchronous::detail::callback_continuation<Policy,Job>
    apply2(Box const& box,
            InputCollection1 const& collection1, index_vector_type const& input1,
            InputCollection2 const& collection2, index_vector_type const& input2,
            int level,
            std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy,
            long partition_cutoff)
    {
        // TODO name +prio
        auto input1_ = std::make_shared<index_vector_type>(std::move(input1));
        auto input2_ = std::make_shared<index_vector_type>(std::move(input2));
        return boost::asynchronous::top_level_callback_continuation_job<Policy,Job>
                 (simple_parallel_partition_task<InputCollection1,InputCollection2,Policy,this_type>(
                      box,
                      std::make_shared<InputCollection1>(collection1),input1_,
                      boost::begin(*input1_),boost::end(*input1_),
                      std::make_shared<InputCollection2>(collection2),input2_,
                      boost::begin(*input2_),boost::end(*input2_),
                      level,min_elements,
                      std::make_shared<Policy>(std::move(policy)),box_policy,
                      partition_cutoff,"geometry::parallel_partition_two",0));
    }

    template
    <
        typename InputCollection1,
        typename InputCollection2,
        typename Policy
    >
    static inline void apply3(Box const& box,
            std::shared_ptr<InputCollection1> collection1, index_iterator_type beg1,index_iterator_type end1,
            std::shared_ptr<InputCollection2> collection2, index_iterator_type beg2,index_iterator_type end2,
            int level,
            std::size_t min_elements,
            std::shared_ptr<Policy> policy, VisitBoxPolicy& box_policy)
    {
        box_policy.apply(box, level);

        Box lower_box, upper_box;
        divide_box<Dimension>(box, lower_box, upper_box);

        index_vector_type lower1, upper1, exceeding1;
        index_vector_type lower2, upper2, exceeding2;
        divide_into_subsets_it<OverlapsPolicy1>(lower_box, upper_box, *collection1,
                    beg1,end1, lower1, upper1, exceeding1);
        divide_into_subsets_it<OverlapsPolicy2>(lower_box, upper_box, *collection2,
                    beg2,end2, lower2, upper2, exceeding2);

        if (boost::size(exceeding1) > 0)
        {
            // All exceeding from 1 with 2:

            if (recurse_ok(exceeding1, exceeding2, min_elements, level))
            {
                Box exceeding_box = get_new_box(*collection1, exceeding1,
                            *collection2, exceeding2);
                next_level(exceeding_box, *collection1, exceeding1,
                                *collection2, exceeding2, level,
                                min_elements, *policy, box_policy);
            }
            else
            {
                handle_two(*collection1, exceeding1, *collection2, exceeding2,
                            *policy);
            }

            // All exceeding from 1 with lower and upper of 2:

            // (Check sizes of all three collections to avoid recurse into
            // the same combinations again and again)
            if (recurse_ok(lower2, upper2, exceeding1, min_elements, level))
            {
                Box exceeding_box
                    = get_new_box<ExpandPolicy1>(*collection1, exceeding1);
                next_level(exceeding_box, *collection1, exceeding1,
                    *collection2, lower2, level, min_elements, *policy, box_policy);
                next_level(exceeding_box, *collection1, exceeding1,
                    *collection2, upper2, level, min_elements, *policy, box_policy);
            }
            else
            {
                handle_two(*collection1, exceeding1, *collection2, lower2, *policy);
                handle_two(*collection1, exceeding1, *collection2, upper2, *policy);
            }
        }

        if (boost::size(exceeding2) > 0)
        {
            // All exceeding from 2 with lower and upper of 1:
            if (recurse_ok(lower1, upper1, exceeding2, min_elements, level))
            {
                Box exceeding_box
                    = get_new_box<ExpandPolicy2>(*collection2, exceeding2);
                next_level(exceeding_box, *collection1, lower1,
                    *collection2, exceeding2, level, min_elements, *policy, box_policy);
                next_level(exceeding_box, *collection1, upper1,
                    *collection2, exceeding2, level, min_elements, *policy, box_policy);
            }
            else
            {
                handle_two(*collection1, lower1, *collection2, exceeding2, *policy);
                handle_two(*collection1, upper1, *collection2, exceeding2, *policy);
            }
        }

        if (recurse_ok(lower1, lower2, min_elements, level))
        {
            next_level(lower_box, *collection1, lower1, *collection2, lower2, level,
                            min_elements, *policy, box_policy);
        }
        else
        {
            handle_two(*collection1, lower1, *collection2, lower2, *policy);
        }
        if (recurse_ok(upper1, upper2, min_elements, level))
        {
            next_level(upper_box, *collection1, upper1, *collection2, upper2, level,
                            min_elements, *policy, box_policy);
        }
        else
        {
            handle_two(*collection1, upper1, *collection2, upper2, *policy);
        }

    }

    template
    <
        typename InputCollection1,
        typename InputCollection2,
        typename Policy
    >
    static inline void apply(Box const& box,
            InputCollection1 const& collection1, index_vector_type const& input1,
            InputCollection2 const& collection2, index_vector_type const& input2,
            int level,
            std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {
        box_policy.apply(box, level);

        Box lower_box, upper_box;
        divide_box<Dimension>(box, lower_box, upper_box);

        index_vector_type lower1, upper1, exceeding1;
        index_vector_type lower2, upper2, exceeding2;
        divide_into_subsets<OverlapsPolicy1>(lower_box, upper_box, collection1,
                    input1, lower1, upper1, exceeding1);
        divide_into_subsets<OverlapsPolicy2>(lower_box, upper_box, collection2,
                    input2, lower2, upper2, exceeding2);

        if (boost::size(exceeding1) > 0)
        {
            // All exceeding from 1 with 2:

            if (recurse_ok(exceeding1, exceeding2, min_elements, level))
            {
                Box exceeding_box = get_new_box(collection1, exceeding1,
                            collection2, exceeding2);
                next_level(exceeding_box, collection1, exceeding1,
                                collection2, exceeding2, level,
                                min_elements, policy, box_policy);
            }
            else
            {
                handle_two(collection1, exceeding1, collection2, exceeding2,
                            policy);
            }

            // All exceeding from 1 with lower and upper of 2:

            // (Check sizes of all three collections to avoid recurse into
            // the same combinations again and again)
            if (recurse_ok(lower2, upper2, exceeding1, min_elements, level))
            {
                Box exceeding_box
                    = get_new_box<ExpandPolicy1>(collection1, exceeding1);
                next_level(exceeding_box, collection1, exceeding1,
                    collection2, lower2, level, min_elements, policy, box_policy);
                next_level(exceeding_box, collection1, exceeding1,
                    collection2, upper2, level, min_elements, policy, box_policy);
            }
            else
            {
                handle_two(collection1, exceeding1, collection2, lower2, policy);
                handle_two(collection1, exceeding1, collection2, upper2, policy);
            }
        }

        if (boost::size(exceeding2) > 0)
        {
            // All exceeding from 2 with lower and upper of 1:
            if (recurse_ok(lower1, upper1, exceeding2, min_elements, level))
            {
                Box exceeding_box
                    = get_new_box<ExpandPolicy2>(collection2, exceeding2);
                next_level(exceeding_box, collection1, lower1,
                    collection2, exceeding2, level, min_elements, policy, box_policy);
                next_level(exceeding_box, collection1, upper1,
                    collection2, exceeding2, level, min_elements, policy, box_policy);
            }
            else
            {
                handle_two(collection1, lower1, collection2, exceeding2, policy);
                handle_two(collection1, upper1, collection2, exceeding2, policy);
            }
        }

        if (recurse_ok(lower1, lower2, min_elements, level))
        {
            next_level(lower_box, collection1, lower1, collection2, lower2, level,
                            min_elements, policy, box_policy);
        }
        else
        {
            handle_two(collection1, lower1, collection2, lower2, policy);
        }
        if (recurse_ok(upper1, upper2, min_elements, level))
        {
            next_level(upper_box, collection1, upper1, collection2, upper2, level,
                            min_elements, policy, box_policy);
        }
        else
        {
            handle_two(collection1, upper1, collection2, upper2, policy);
        }
    }
};

template
<
    int Dimension,
    typename Box,
    typename OverlapsPolicy,
    typename ExpandPolicy,
    typename VisitBoxPolicy,
    typename Job
>
class parallel_partition_one_collection
{
    typedef std::vector<std::size_t> index_vector_type;
    typedef parallel_partition_one_collection this_type;
    template<int, class,class,class,class,class> friend class parallel_partition_one_collection;


    template <typename InputCollection>
    static inline Box get_new_box(InputCollection const& collection,
                    index_vector_type const& input)
    {
        Box box;
        geometry::assign_inverse(box);
        expand_with_elements<ExpandPolicy>(box, collection, input);
        return box;
    }

    typedef parallel_partition_one_collection
            <
                1 - Dimension,
                Box,
                OverlapsPolicy,
                ExpandPolicy,
                VisitBoxPolicy,
                Job
            > sub_divide;

    typedef boost::range_iterator
        <
            index_vector_type const
        >::type index_iterator_type;

    template <typename InputCollection, typename Policy>
    static inline void next_level(Box const& box,
            InputCollection const& collection,
            index_vector_type const& input,
            std::size_t level, std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {
        if (recurse_ok(input, min_elements, level))
        {
            partition_one_collection
            <
                1 - Dimension,
                Box,
                OverlapsPolicy,
                ExpandPolicy,
                VisitBoxPolicy
            >::apply(box, collection, input,
                level + 1, min_elements, policy, box_policy);
        }
        else
        {
            handle_one(collection, input, policy);
        }
    }

    // Function to switch to two collections if there are geometries exceeding
    // the separation line
    template <typename InputCollection, typename Policy>
    static inline void next_level2(Box const& box,
            InputCollection const& collection,
            index_vector_type const& input1,
            index_vector_type const& input2,
            std::size_t level, std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {

        if (recurse_ok(input1, input2, min_elements, level))
        {
            partition_two_collections
            <
                1 - Dimension,
                Box,
                OverlapsPolicy, OverlapsPolicy,
                ExpandPolicy, ExpandPolicy,
                VisitBoxPolicy
            >::apply(box, collection, input1, collection, input2,
                level + 1, min_elements, policy, box_policy);
        }
        else
        {
            handle_two(collection, input1, collection, input2, policy);
        }
    }

public :

    template
    <
        typename InputCollection,
        typename Policy
    >
    struct next_level_2_task : public boost::asynchronous::continuation_task<Policy>
    {
        next_level_2_task(Box const& box,
                                std::shared_ptr<InputCollection> collection,
                                std::shared_ptr<index_vector_type> input1,
                                std::shared_ptr<index_vector_type> input2,
                                int level,
                                std::size_t min_elements,
                                std::shared_ptr<Policy> policy, VisitBoxPolicy& box_policy,
                                long cutoff,const std::string& task_name, std::size_t prio)
            : boost::asynchronous::continuation_task<Policy>(task_name)
            // TODO move
            , box_(box)
            , collection_(collection)
            , input1_(input1)
            , input2_(input2)
            , level_(level)
            , min_elements_(min_elements),policy_(policy),box_policy_(box_policy)
            , cutoff_(cutoff), prio_(prio)
        {}

        void operator()()
        {
            boost::asynchronous::continuation_result<Policy> task_res = this->this_task_result();
            if (recurse_ok(*input1_, *input2_, cutoff_, level_))
            {
                //auto policy = *policy_;
                auto policy = policy_;
                // TODO remove unnecessary copy of inputs in apply2
                /*auto cont =
                parallel_partition_two_collections
                <
                    1 - Dimension,
                    Box,
                    OverlapsPolicy, OverlapsPolicy,
                    ExpandPolicy, ExpandPolicy,
                    VisitBoxPolicy,Job
                >::apply2(box_, *collection_, *input1_, *collection_, *input2_,
                    level_ + 1, min_elements_, *policy_, box_policy_,cutoff_);

                cont.on_done([task_res](std::tuple<boost::asynchronous::expected<Policy>>&& continuation_res)
                {
                    try
                    {
                        task_res.set_value(std::move(std::get<0>(continuation_res).get()));
                    }
                    catch(std::exception& e)
                    {
                        task_res.set_exception(std::make_exception_ptr(e));
                    }
                }
                );*/
                partition_two_collections
                <
                    1 - Dimension,
                    Box,
                    OverlapsPolicy, OverlapsPolicy,
                    ExpandPolicy, ExpandPolicy,
                    VisitBoxPolicy
                >::apply(box_, *collection_, *input1_, *collection_, *input2_,
                    level_ + 1, min_elements_, *policy_, box_policy_);

                handle_two(*collection_, *input1_, *collection_, *input2_, *policy_);
                task_res.set_value(*policy_);
            }
            else
            {
                handle_two(*collection_, *input1_, *collection_, *input2_, *policy_);
                task_res.set_value(*policy_);
            }
        }

        Box box_;
        std::shared_ptr<InputCollection> collection_;
        std::shared_ptr<index_vector_type> input1_;
        std::shared_ptr<index_vector_type> input2_;
        int level_;
        std::size_t min_elements_;
        std::shared_ptr<Policy> policy_;
        VisitBoxPolicy box_policy_;
        long cutoff_;
        std::size_t prio_;
    };

    template
    <
        typename InputCollection,
        typename Policy,
        typename Owner
    >
    struct simple_parallel_partition_task : public boost::asynchronous::continuation_task<Policy>
    {
        simple_parallel_partition_task(Box const& box,
                                std::shared_ptr<InputCollection> collection,std::shared_ptr<index_vector_type> input,
                                int level,
                                std::size_t min_elements,
                                std::shared_ptr<Policy> policy, VisitBoxPolicy& box_policy,
                                long cutoff,const std::string& task_name, std::size_t prio)
            : boost::asynchronous::continuation_task<Policy>(task_name)
            // TODO move
            , box_(box)
            , collection_(collection)
            , input_(input)
            , level_(level)
            , min_elements_(min_elements),policy_(policy),box_policy_(box_policy)
            , cutoff_(cutoff), prio_(prio)
        {}

        void operator()()
        {
            boost::asynchronous::continuation_result<Policy> task_res = this->this_task_result();
            box_policy_.apply(box_, level_);

            Box lower_box, upper_box;
            divide_box<Dimension>(box_, lower_box, upper_box);

            index_vector_type lower, upper, exceeding;
            divide_into_subsets<OverlapsPolicy>(lower_box, upper_box, *collection_,
                                                *input_, lower, upper, exceeding);
            if (boost::size(exceeding) > 0)
            {
                // Get the box of exceeding-only
                Box exceeding_box = get_new_box(*collection_, exceeding);

                // Recursively do exceeding elements only, in next dimension they
                // will probably be less exceeding within the new box
                Owner::next_level(exceeding_box, *collection_, exceeding, level_,
                    min_elements_, *policy_, box_policy_);

/*              auto p1 = std::make_shared<Policy>(policy_->clone());
                auto p2 = std::make_shared<Policy>(policy_->clone());

                next_level2(exceeding_box, *collection_, exceeding, lower, level_,
                                    min_elements_, *p1, box_policy_);
                next_level2(exceeding_box, *collection_, exceeding, upper, level_,
                                    min_elements_, *p2, box_policy_);

                 (*policy_).merge(*p1);
                 (*policy_).merge(*p2);
*/
                // Switch to two collections, combine exceeding with lower resp upper
                // but not lower/lower, upper/upper
                next_level2(exceeding_box, *collection_, exceeding, lower, level_,
                    min_elements_, *policy_, box_policy_);
                next_level2(exceeding_box, *collection_, exceeding, upper, level_,
                    min_elements_, *policy_, box_policy_);

               /* std::shared_ptr<index_vector_type> exceeding_ = std::make_shared<index_vector_type>(std::move(exceeding));
                std::shared_ptr<index_vector_type> lower_ = std::make_shared<index_vector_type>(std::move(lower));
                std::shared_ptr<index_vector_type> upper_ = std::make_shared<index_vector_type>(std::move(upper));
                auto policy = policy_;
                auto collection = collection_;
                auto level = level_;
                auto min_elements = min_elements_;
                auto box_policy = box_policy_;
                //auto name = this->get_name();
                //auto cutoff = cutoff_;
                //auto prio = prio_;

                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res,policy,lower_box,upper_box,collection,lower_,upper_,level,min_elements,box_policy]
                            (std::tuple<boost::asynchronous::expected<Policy>,
                                        boost::asynchronous::expected<Policy> >&& res) mutable
                            {
                                try
                                {
                                    auto nl1 = std::move(std::get<0>(res).get());
                                    auto nl2 = std::move(std::get<1>(res).get());
                                    (*policy).merge(nl1);
                                    (*policy).merge(nl2);

                                    // Recursively call operation both parts
                                    Owner::next_level(lower_box, *collection, *lower_, level, min_elements,
                                                    *policy, box_policy);
                                    Owner::next_level(upper_box, *collection, *upper_, level, min_elements,
                                                    *policy, box_policy);

                                    task_res.set_value(std::move(*policy));
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(std::make_exception_ptr(e));
                                }
                            },
                            // recursive tasks
                            next_level_2_task<InputCollection,Policy>
                                (exceeding_box,
                                 collection_,exceeding_,lower_,
                                 level_,min_elements_,std::make_shared<Policy>(policy_->clone()),box_policy_,cutoff_,this->get_name(),prio_),
                            next_level_2_task<InputCollection,Policy>
                                (exceeding_box,
                                 collection_,exceeding_,upper_,
                                 level_,min_elements_,std::make_shared<Policy>(policy_->clone()),box_policy_,cutoff_,this->get_name(),prio_)
                   );*/
/*
                boost::asynchronous::create_callback_continuation_job<Job>(
                            // called when subtasks are done, set our result
                            [task_res,prio,cutoff,exceeding_box,policy,lower_box,upper_box,collection,lower_,upper_,exceeding_,level,min_elements,box_policy,name]
                            (std::tuple<boost::asynchronous::expected<Policy>>&& res) mutable
                            {
                                try
                                {
                                    auto nl1 = std::move(std::get<0>(res).get());
                                    (*policy).merge(nl1);

                                    boost::asynchronous::create_callback_continuation_job<Job>(
                                                // called when subtasks are done, set our result
                                                [task_res,prio,cutoff,policy,exceeding_box,lower_box,upper_box,collection,lower_,exceeding_,upper_,level,min_elements,box_policy]
                                                (std::tuple<boost::asynchronous::expected<Policy>>&& res) mutable
                                                {
                                                    try
                                                    {
                                                        auto nl1 = std::move(std::get<0>(res).get());
                                                        (*policy).merge(nl1);

                                                        // Recursively call operation both parts
                                                        Owner::next_level(lower_box, *collection, *lower_, level, min_elements,
                                                                        *policy, box_policy);
                                                        Owner::next_level(upper_box, *collection, *upper_, level, min_elements,
                                                                        *policy, box_policy);

                                                        task_res.set_value(std::move(*policy));
                                                    }
                                                    catch(std::exception& e)
                                                    {
                                                        task_res.set_exception(std::make_exception_ptr(e));
                                                    }
                                                },
                                                // recursive tasks
                                                next_level_2_task<InputCollection,Policy>
                                                    (exceeding_box,
                                                     collection,exceeding_,upper_,
                                                     level,min_elements,std::make_shared<Policy>(policy->clone()),box_policy,cutoff,name,prio)
                                       );
                                }
                                catch(std::exception& e)
                                {
                                    task_res.set_exception(std::make_exception_ptr(e));
                                }
                            },
                            // recursive tasks
                            next_level_2_task<InputCollection,Policy>
                                (exceeding_box,
                                 collection_,exceeding_,lower_,
                                 level_,min_elements_,std::make_shared<Policy>(policy_->clone()),box_policy_,cutoff_,this->get_name(),prio_)
                   );*/
            }
          //  else
            {
                // Recursively call operation both parts
                next_level(lower_box, *collection_, lower, level_, min_elements_,
                                *policy_, box_policy_);
                next_level(upper_box, *collection_, upper, level_, min_elements_,
                                *policy_, box_policy_);

                task_res.set_value(std::move(*policy_));
            }
        }

        Box box_;
        std::shared_ptr<InputCollection> collection_;
        std::shared_ptr<index_vector_type> input_;
        int level_;
        std::size_t min_elements_;
        std::shared_ptr<Policy> policy_;
        VisitBoxPolicy box_policy_;
        long cutoff_;
        std::size_t prio_;
    };

    template
    <
        typename InputCollection,
        typename Policy
    >
    static inline
    boost::asynchronous::detail::callback_continuation<Policy,Job>
    apply2(Box const& box,
            InputCollection const& collection, index_vector_type const& input,
            int level,
            std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy,
            long partition_cutoff)
    {
        // TODO name +prio
        auto input_ = std::make_shared<index_vector_type>(std::move(input));
        return boost::asynchronous::top_level_callback_continuation_job<Policy,Job>
                 (simple_parallel_partition_task<InputCollection,Policy,this_type>(
                      box,
                      std::make_shared<InputCollection>(collection),input_,
                      level,min_elements,
                      std::make_shared<Policy>(std::move(policy)),box_policy,
                      partition_cutoff,"geometry::parallel_partition_one",0));
    }

    template
    <
        typename InputCollection,
        typename Policy
    >
    static inline void apply3(Box const& box,
            std::shared_ptr<InputCollection> collection, index_iterator_type beg,index_iterator_type end,
            int level,
            std::size_t min_elements,
            std::shared_ptr<Policy> policy, VisitBoxPolicy& box_policy)
    {
        box_policy.apply(box, level);

        Box lower_box, upper_box;
        divide_box<Dimension>(box, lower_box, upper_box);
        index_vector_type lower, upper, exceeding;
        divide_into_subsets_it<OverlapsPolicy>(lower_box, upper_box, *collection,
                    beg,end, lower, upper, exceeding);
        if (boost::size(exceeding) > 0)
        {
            // Get the box of exceeding-only
            Box exceeding_box = get_new_box(*collection, exceeding);

            // Recursively do exceeding elements only, in next dimension they
            // will probably be less exceeding within the new box
            next_level(exceeding_box, *collection, exceeding, level,
                min_elements, *policy, box_policy);

            // Switch to two collections, combine exceeding with lower resp upper
            // but not lower/lower, upper/upper
            next_level2(exceeding_box, *collection, exceeding, lower, level,
                min_elements, *policy, box_policy);
            next_level2(exceeding_box, *collection, exceeding, upper, level,
                min_elements, *policy, box_policy);
        }

        // Recursively call operation both parts
        next_level(lower_box, *collection, lower, level, min_elements,
                        *policy, box_policy);
        next_level(upper_box, *collection, upper, level, min_elements,
                        *policy, box_policy);
    }

    template <typename InputCollection, typename Policy>
    static inline void apply(Box const& box,
            InputCollection const& collection,
            index_vector_type const& input,
            std::size_t level,
            std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {
        box_policy.apply(box, level);

        Box lower_box, upper_box;
        divide_box<Dimension>(box, lower_box, upper_box);

        index_vector_type lower, upper, exceeding;
        divide_into_subsets<OverlapsPolicy>(lower_box, upper_box, collection,
                    input, lower, upper, exceeding);
        if (boost::size(exceeding) > 0)
        {
            // Get the box of exceeding-only
            Box exceeding_box = get_new_box(collection, exceeding);

            // Recursively do exceeding elements only, in next dimension they
            // will probably be less exceeding within the new box
            next_level(exceeding_box, collection, exceeding, level,
                min_elements, policy, box_policy);

            // Switch to two collections, combine exceeding with lower resp upper
            // but not lower/lower, upper/upper
            next_level2(exceeding_box, collection, exceeding, lower, level,
                min_elements, policy, box_policy);
            next_level2(exceeding_box, collection, exceeding, upper, level,
                min_elements, policy, box_policy);
        }

        // Recursively call operation both parts
        next_level(lower_box, collection, lower, level, min_elements,
                        policy, box_policy);
        next_level(upper_box, collection, upper, level, min_elements,
                        policy, box_policy);
    }
};

}} // namespace detail::partition

template
<
    typename Box,
    typename ExpandPolicy1,
    typename OverlapsPolicy1,
    typename Job,
    typename ExpandPolicy2 = ExpandPolicy1,
    typename OverlapsPolicy2 = OverlapsPolicy1,
    typename IncludePolicy1 = detail::partition::include_all_policy,
    typename IncludePolicy2 = detail::partition::include_all_policy,
    typename VisitBoxPolicy = boost::geometry::detail::partition::visit_no_policy   
>
class parallel_partition
{
    typedef std::vector<std::size_t> index_vector_type;

    template <typename ExpandPolicy, typename IncludePolicy, typename InputCollection>
    static inline void expand_to_collection(InputCollection const& collection,
                Box& total, index_vector_type& index_vector)
    {
        std::size_t index = 0;
        for(typename boost::range_iterator<InputCollection const>::type it
            = boost::begin(collection);
            it != boost::end(collection);
            ++it, ++index)
        {
            if (IncludePolicy::apply(*it))
            {
                ExpandPolicy::apply(total, *it);
                index_vector.push_back(index);
            }
        }
    }

public :
    template <typename InputCollection, typename VisitPolicy>
    static inline boost::asynchronous::detail::callback_continuation<VisitPolicy,Job>
    apply(InputCollection const& collection,
            VisitPolicy& visitor,
            long partition_cutoff,
            std::size_t min_elements = 16,
            VisitBoxPolicy box_visitor = boost::geometry::detail::partition::visit_no_policy()
            )
    {
        index_vector_type index_vector;
        Box total;
        assign_inverse(total);
        expand_to_collection<ExpandPolicy1, IncludePolicy1>(collection, total, index_vector);

        return
        detail::partition::parallel_partition_one_collection
            <
                0, Box,
                OverlapsPolicy1,
                ExpandPolicy1,
                VisitBoxPolicy,
                Job
            >::apply2(total, collection, index_vector, 0, min_elements,
                            visitor, box_visitor, partition_cutoff);
    }

    template
    <
        typename InputCollection1,
        typename InputCollection2,
        typename VisitPolicy
    >
    static inline boost::asynchronous::detail::callback_continuation<VisitPolicy,Job>
    apply(InputCollection1 const& collection1,
          InputCollection2 const& collection2,
          VisitPolicy& visitor,
          long partition_cutoff,
          std::size_t min_elements = 16,
          VisitBoxPolicy box_visitor = detail::partition::visit_no_policy())
    {
        index_vector_type index_vector1, index_vector2;
        Box total;
        assign_inverse(total);
        expand_to_collection<ExpandPolicy1, IncludePolicy1>(collection1,
                total, index_vector1);
        expand_to_collection<ExpandPolicy2, IncludePolicy2>(collection2,
                total, index_vector2);

        return
        detail::partition::parallel_partition_two_collections
            <
                0, Box, OverlapsPolicy1, OverlapsPolicy2,
                ExpandPolicy1, ExpandPolicy2, VisitBoxPolicy,Job
            >::apply2(total,
                collection1, index_vector1,
                collection2, index_vector2,
                0, min_elements, visitor, box_visitor, partition_cutoff);
    }
};



}} // namespace boost::geometry

#endif // BOOST_ASYNCHRONOUS_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP
