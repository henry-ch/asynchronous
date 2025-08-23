#include <iostream>
#include <boost/asynchronous/notification/topics.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;

namespace
{
    using topic_t = boost::asynchronous::subscription::channel_topic<>;
}

BOOST_AUTO_TEST_CASE( test_channel_topic_exact_match )
{      
    {
        topic_t subscribed_topic{ "ChannelA/" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches(topic_t{ "ChannelA/" }), "not matching equal");
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/"), "not matching equal w/ string");
    }
    {
        topic_t subscribed_topic{ "ChannelA/ChannelB/" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelB"), "not matching equal w/ string");
    }
    {
        topic_t subscribed_topic{ "ChannelA/ChannelB" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelB/"), "not matching equal w/ string");
    }
}

BOOST_AUTO_TEST_CASE(test_channel_topic_sub_match)
{
    {
        topic_t subscribed_topic{ "ChannelA/ChannelB" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelB/ChannelC"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/ChannelB" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelB/ChannelC/ChannelD"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/ChannelB" };
        BOOST_CHECK_MESSAGE(!subscribed_topic.matches("ChannelA/ChannelBish"), "matching wrong subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelB"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelB"), "not matching subchannel");
    }
}

BOOST_AUTO_TEST_CASE(test_channel_topic_wildcard_match)
{
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB/"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB/" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB/"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB/" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB/" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB/"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB/" };
        BOOST_CHECK_MESSAGE(!subscribed_topic.matches("ChannelA/ChannelX/ChannelBish"), "matching wrong subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB" };
        BOOST_CHECK_MESSAGE(!subscribed_topic.matches("ChannelA/ChannelX/ChannelBish"), "matching wrong subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB/" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB/ChannelC"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB/" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB/ChannelC/"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB/ChannelC/"), "not matching subchannel");
    }
    {
        topic_t subscribed_topic{ "ChannelA/./ChannelB" };
        BOOST_CHECK_MESSAGE(subscribed_topic.matches("ChannelA/ChannelX/ChannelB/ChannelC"), "not matching subchannel");
    }
}


