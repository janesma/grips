#include "gtest/gtest.h" 
#include "gfcpu_provider.h"
#include "gfpublisher.h"
#include "gfsubscriber_remote.h"

namespace Grafips
{

    class NullPublisher : public PublisherInterface
    {
      public:
        void RegisterProvider(Provider *p) {}
        void OnMetric(const DataSet &d) {}
        void Enable(int id) {}
        void Disable(int id) {}
        void GetDescriptions(std::vector<MetricDescription> *descriptions) const {}
        void Subscribe(SubscriberInterface *) {}
    };

    class TestPublisher : public PublisherInterface
    {
      public:
        void RegisterProvider(Provider *p) { m_p = p; }
        void OnMetric(const DataSet &d) {m_d.insert(m_d.end(), d.begin(), d.end()); }
        void Enable(int id) { m_p->Enable(id); }
        virtual void Disable(int id) { m_p->Disable(id); }
        void GetDescriptions(std::vector<MetricDescription> *descriptions) const
            { m_p->GetDescriptions(descriptions); }
        void Subscribe(SubscriberInterface *) {}
        DataSet m_d;
        Provider *m_p;
    };



    class CpuProviderFixture : public testing::Test
    {
      public:
        CpuProviderFixture() 
            {
            }
      protected:
        virtual void SetUp() {}
        virtual void TearDown() {}

        void test_parse()
            {
                NullPublisher pub;
                CpuProvider p;
                p.setPublisher(&pub);
                EXPECT_GT(p.m_systemStats.user, 0);
                EXPECT_GT(p.m_systemStats.system, 0);
                EXPECT_GT(p.m_systemStats.idle, 0);

                EXPECT_GT(p.m_core_stats.size(), 0);
                EXPECT_GT(p.m_core_stats[0].user, 0);
                EXPECT_GT(p.m_core_stats[0].system, 0);
                EXPECT_GT(p.m_core_stats[0].idle, 0);
            }

        void test_publish()
            {
                TestPublisher pub;
                CpuProvider p;
                p.setPublisher(&pub);

                MetricDescriptionSet metrics;
                pub.GetDescriptions(&metrics);
                for (MetricDescriptionSet::iterator i = metrics.begin(); i != metrics.end(); ++i)
                    pub.Enable(i->id());
                usleep(100000);
                p.Poll();
                EXPECT_EQ(pub.m_d.size(), metrics.size());
                for (int i = 0; i < pub.m_d.size(); ++i)
                {
                    EXPECT_LT(pub.m_d[i].data, 100);
                    EXPECT_GE(pub.m_d[i].data, 0);
                    for (int j = i + 1; j < pub.m_d.size(); ++j)
                    {
                        EXPECT_NE(pub.m_d[i].id, pub.m_d[j].id);
                        EXPECT_EQ(pub.m_d[i].time_val, pub.m_d[j].time_val);
                    }
                }
            }
    };

    TEST_F(CpuProviderFixture, test_parse )
    {
        test_parse();
    }

    TEST_F(CpuProviderFixture, test_publish )
    {
        test_publish();
    }


    class SubscriberMock: public SubscriberInterface
    {
      public:
        SubscriberMock() : m_cleared(false), m_clear_arg(-1) {}
        void Clear(int id) { m_cleared = true; m_clear_arg = id; }
        void OnMetric(const DataSet &d) {}
        void OnDescriptions(const std::vector<MetricDescription> &descriptions) {}
        bool m_cleared;
        int m_clear_arg;
    };

    class RemoteInvokeTest : public testing::Test
    {
      public:
        RemoteInvokeTest() : m_stub(NULL),
                             m_skel(NULL) {}
      protected:
        virtual void SetUp()
            {
                m_skel = new SubscriberSkeleton(53134, &m_mock);
                m_skel->Start();
                m_stub = new SubscriberStub("localhost", 53134);
            }
        virtual void TearDown()
            {
                m_skel->Stop();

                // send message to trigger server stop
                delete m_stub;

                m_skel->Join();
                delete m_skel;
            }

        SubscriberStub *m_stub;
        SubscriberSkeleton *m_skel;
        SubscriberMock m_mock;
    };


    TEST_F(RemoteInvokeTest, test_call_clear )
    {
        ASSERT_FALSE(m_mock.m_cleared);
        m_stub->Clear(5);
        EXPECT_TRUE(m_mock.m_cleared);
        EXPECT_EQ(m_mock.m_clear_arg, 5);
    }
}
