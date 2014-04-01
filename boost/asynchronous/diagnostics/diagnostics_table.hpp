// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_DIAGNOSTICS_DIAGNOSTICS_TABLE_HPP
#define BOOST_ASYNC_DIAGNOSTICS_DIAGNOSTICS_TABLE_HPP

#include <memory>
#include <vector>
#include <list>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition.hpp>

namespace boost { namespace asynchronous
{

template<typename Key,typename Value,typename Hash=std::hash<Key> >
class diagnostics_table
{
private:
    struct bucket_type
    {
        typedef std::pair<Key,Value> bucket_value;
        typedef std::list<bucket_value> bucket_data;
        typedef typename bucket_data::iterator bucket_iterator;
        bucket_data data;
        mutable boost::shared_mutex mutex;

        void add(Key const& key,Value const& value)
        {
            boost::unique_lock<boost::shared_mutex> lock(mutex);
            data.push_back(bucket_value(key,value));
        }
    };
    // TODO unique_ptr bug?
    std::vector<boost::shared_ptr<bucket_type> > m_buckets;
    Hash m_hasher;

    bucket_type& get_bucket(Key const& key) const
    {
        std::size_t const bucket_index=m_hasher(key)%m_buckets.size();
        return *m_buckets[bucket_index];
    }
public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Hash hash_type;
    diagnostics_table(
            // TODO magic number
            unsigned num_buckets=19,Hash const& hasher_=Hash()):
        m_buckets(num_buckets),m_hasher(hasher_)
    {
        //m_buckets.reserve(num_buckets);
        for(unsigned i=0;i<num_buckets;++i)
        {
            m_buckets[i].reset(new bucket_type);
            //std::unique_ptr<bucket_type>p(new bucket_type);
            //m_buckets.push_back(std::move(p));
        }
    }
    diagnostics_table(diagnostics_table const& other)=delete;
    diagnostics_table& operator=(
            diagnostics_table const& other)=delete;

    void add(Key const& key,Value const& value)
    {
        get_bucket(key).add(key,value);
    }
    // TODO chose returned sequence container
    std::map<Key,std::list<Value> > get_map() const
    {
        std::vector<boost::unique_lock<boost::shared_mutex> > locks;
        for(unsigned i=0;i<m_buckets.size();++i)
        {
            locks.push_back(
                        boost::unique_lock<boost::shared_mutex>((*m_buckets[i]).mutex));
        }
        std::map<Key,std::list<Value> > res;
        for(unsigned i=0;i<m_buckets.size();++i)
        {
            for(typename bucket_type::bucket_iterator it=(*m_buckets[i]).data.begin();
                it!=(*m_buckets[i]).data.end();
                ++it)
            {
                res[(*it).first].push_back((*it).second);
            }
        }
        return res;
    }
    void clear()
    {
        std::vector<boost::unique_lock<boost::shared_mutex> > locks;
        for(unsigned i=0;i<m_buckets.size();++i)
        {
            locks.push_back(
                        boost::unique_lock<boost::shared_mutex>((*m_buckets[i]).mutex));
        }
        for(unsigned i=0;i<m_buckets.size();++i)
        {
            (*m_buckets[i]).data.clear();
        }
    }

};
}} // boost::asynchronous

#endif // BOOST_ASYNC_DIAGNOSTICS_DIAGNOSTICS_TABLE_HPP
