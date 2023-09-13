#include "agent_list.h"

#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/size_literals.h>

#include <chrono>

namespace NCloud::NBlockStore::NStorage {

using namespace std::chrono_literals;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TDeviceConfig CreateDevice(TString id, ui64 size)
{
    NProto::TDeviceConfig config;
    config.SetDeviceName("name-" + id);
    config.SetDeviceUUID(id);
    config.SetBlockSize(4_KB);
    config.SetBlocksCount(size / 4_KB);
    config.SetTransportId("transport-" + id);
    config.SetBaseName("base-name");
    config.SetRack("the-rack");
    config.MutableRdmaEndpoint()->SetHost("rdma-" + id);
    config.MutableRdmaEndpoint()->SetPort(10020);

    return config;
}

NProto::TDeviceConfig CreateDevice(TString id)
{
    return CreateDevice(std::move(id), 1_GB);
}

////////////////////////////////////////////////////////////////////////////////

struct TAgentListParams
{
    TAgentListConfig Config;
    TVector<NProto::TAgentConfig> Agents;
    THashMap<TString, NProto::TDiskRegistryAgentParams> DiskRegistryAgentListParams;
};

struct TFixture
    : public NUnitTest::TBaseFixture
{
    IMonitoringServicePtr Monitoring = CreateMonitoringServiceStub();
    NMonitoring::TDynamicCounterPtr Counters = Monitoring->GetCounters()
        ->GetSubgroup("counters", "blockstore")
        ->GetSubgroup("component", "disk_registry");

    ILoggingServicePtr Logging = CreateLoggingService("console");

    TAgentList CreateAgentList(TAgentListParams params = {})
    {
        return TAgentList {
            std::move(params.Config),
            Counters,
            std::move(params.Agents),
            std::move(params.DiskRegistryAgentListParams),
            Logging->CreateLog("BLOCKSTORE_DISK_REGISTRY")
        };
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TAgentListTest)
{
    Y_UNIT_TEST_F(ShouldRegisterAgent, TFixture)
    {
        TAgentList agentList = CreateAgentList();

        UNIT_ASSERT_VALUES_EQUAL(0, agentList.GetAgents().size());
        UNIT_ASSERT(agentList.FindAgent(42) == nullptr);
        UNIT_ASSERT(agentList.FindAgent("unknown") == nullptr);
        UNIT_ASSERT_VALUES_EQUAL(0, agentList.FindNodeId("unknown"));

        UNIT_ASSERT(!agentList.RemoveAgent("unknown"));
        UNIT_ASSERT(!agentList.RemoveAgent(42));

        NProto::TAgentConfig expectedConfig;

        expectedConfig.SetAgentId("foo");
        expectedConfig.SetNodeId(1000);
        *expectedConfig.AddDevices() = CreateDevice("uuid-1");
        *expectedConfig.AddDevices() = CreateDevice("uuid-2");
        *expectedConfig.AddDevices() = CreateDevice("uuid-3");

        const TKnownAgent knownAgent {
            .Devices = {
                { "uuid-1", CreateDevice("uuid-1", 0) },
                { "uuid-2", CreateDevice("uuid-2", 0) },
                { "uuid-3", CreateDevice("uuid-3", 0) }
            }};

        {
            auto r = agentList.RegisterAgent(
                expectedConfig,
                TInstant::FromValue(1),
                knownAgent);

            NProto::TAgentConfig& agent = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(expectedConfig.GetAgentId(), agent.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(expectedConfig.GetNodeId(), agent.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(expectedConfig.DevicesSize(), agent.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(3, r.NewDevices.size());
        }

        auto* agent = agentList.FindAgent(expectedConfig.GetNodeId());

        UNIT_ASSERT(agent != nullptr);
        UNIT_ASSERT_EQUAL(agent, agentList.FindAgent(expectedConfig.GetAgentId()));

        UNIT_ASSERT_VALUES_EQUAL(
            expectedConfig.GetNodeId(),
            agentList.FindNodeId(expectedConfig.GetAgentId()));

        UNIT_ASSERT_VALUES_EQUAL(expectedConfig.DevicesSize(), agent->DevicesSize());
    }

    Y_UNIT_TEST_F(ShouldRegisterAgentAtNewNode, TFixture)
    {
        NProto::TAgentConfig config;

        config.SetAgentId("foo");
        config.SetNodeId(1000);
        *config.AddDevices() = CreateDevice("uuid-1");
        *config.AddDevices() = CreateDevice("uuid-2");
        *config.AddDevices() = CreateDevice("uuid-3");

        TAgentList agentList = CreateAgentList({
            .Agents = {config}
        });

        UNIT_ASSERT_VALUES_EQUAL(1, agentList.GetAgents().size());

        {
            auto* agent = agentList.FindAgent(config.GetNodeId());

            UNIT_ASSERT(agent != nullptr);
            UNIT_ASSERT_EQUAL(agent, agentList.FindAgent(config.GetAgentId()));

            UNIT_ASSERT_VALUES_EQUAL(
                config.GetNodeId(),
                agentList.FindNodeId(config.GetAgentId()));
        }

        config.SetNodeId(2000);

        auto r = agentList.RegisterAgent(
            config,
            TInstant::FromValue(1),
            TKnownAgent {});

        UNIT_ASSERT_VALUES_EQUAL(0, r.NewDevices.size());
        UNIT_ASSERT_VALUES_EQUAL(1, agentList.GetAgents().size());

        {
            auto* agent = agentList.FindAgent(config.GetNodeId());

            UNIT_ASSERT(agent != nullptr);
            UNIT_ASSERT_EQUAL(agent, agentList.FindAgent(config.GetAgentId()));

            UNIT_ASSERT_VALUES_EQUAL(
                config.GetNodeId(),
                agentList.FindNodeId(config.GetAgentId()));
        }
    }

    Y_UNIT_TEST_F(ShouldKeepRegistryDeviceFieldsUponAgentReRegistration, TFixture)
    {
        TAgentList agentList = CreateAgentList();

        NProto::TAgentConfig expectedConfig;

        expectedConfig.SetAgentId("foo");
        expectedConfig.SetNodeId(1000);
        *expectedConfig.AddDevices() = CreateDevice("uuid-1", 2_GB);
        *expectedConfig.AddDevices() = CreateDevice("uuid-2", 2_GB);

        const TKnownAgent knownAgent {
            .Devices = {{ "uuid-1", CreateDevice("uuid-1", 0) }}
        };

        {
            auto r = agentList.RegisterAgent(
                expectedConfig,
                TInstant::FromValue(1),
                knownAgent);

            NProto::TAgentConfig& agent = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(expectedConfig.GetAgentId(), agent.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(expectedConfig.GetNodeId(), agent.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(1, agent.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(1, agent.UnknownDevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(1, r.NewDevices.size());
            UNIT_ASSERT_VALUES_EQUAL(
                "name-uuid-1",
                agent.GetDevices(0).GetDeviceName()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-1",
                agent.GetDevices(0).GetDeviceUUID()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                2_GB / 4_KB,
                agent.GetDevices(0).GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                2_GB / 4_KB,
                agent.GetDevices(0).GetUnadjustedBlockCount()
            );

            UNIT_ASSERT_VALUES_EQUAL(
                "name-uuid-2",
                agent.GetUnknownDevices(0).GetDeviceName()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-2",
                agent.GetUnknownDevices(0).GetDeviceUUID()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                2_GB / 4_KB,
                agent.GetUnknownDevices(0).GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                0,
                agent.GetUnknownDevices(0).GetUnadjustedBlockCount()
            );

            agent.MutableDevices(0)->SetBlocksCount(1_GB / 4_KB);
            // agent.MutableDevices(0)->SetState(NProto::DEVICE_STATE_WARNING);
            agent.MutableDevices(0)->SetStateTs(111);
            agent.MutableDevices(0)->SetCmsTs(222);
            agent.MutableDevices(0)->SetStateMessage("the-message");
        }

        {
            auto r = agentList.RegisterAgent(
                expectedConfig,
                TInstant::FromValue(2),
                knownAgent);

            NProto::TAgentConfig& agent = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(expectedConfig.GetAgentId(), agent.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(expectedConfig.GetNodeId(), agent.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(1, agent.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(1, agent.UnknownDevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, r.NewDevices.size());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-1",
                agent.GetDevices(0).GetDeviceUUID()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-1",
                agent.GetDevices(0).GetDeviceUUID()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                1_GB / 4_KB,
                agent.GetDevices(0).GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                2_GB / 4_KB,
                agent.GetDevices(0).GetUnadjustedBlockCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                111,
                agent.GetDevices(0).GetStateTs()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                222,
                agent.GetDevices(0).GetCmsTs()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "the-message",
                agent.GetDevices(0).GetStateMessage()
            );

            // these fields should be taken from agent's data
            UNIT_ASSERT_VALUES_EQUAL("the-rack", agent.GetDevices(0).GetRack());
            UNIT_ASSERT_VALUES_EQUAL(
                "base-name",
                agent.GetDevices(0).GetBaseName()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "transport-uuid-1",
                agent.GetDevices(0).GetTransportId()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                "rdma-uuid-1",
                agent.GetDevices(0).GetRdmaEndpoint().GetHost()
            );
        }
    }

    Y_UNIT_TEST_F(ShouldUpdateDevices, TFixture)
    {
        TAgentList agentList = CreateAgentList({
            .Agents = [] {
                NProto::TAgentConfig foo;

                foo.SetAgentId("foo");
                foo.SetNodeId(1000);
                *foo.AddDevices() = CreateDevice("x");
                *foo.AddDevices() = CreateDevice("y");

                return TVector{foo};
            }()
        });

        const TKnownAgent knownAgent {
            .Devices = {
                { "x", CreateDevice("x", 0) },
                { "y", CreateDevice("y", 0) },
                { "z", CreateDevice("z", 0) }
            }};

        {
            auto* foo = agentList.FindAgent("foo");
            UNIT_ASSERT_VALUES_UNEQUAL(nullptr, foo);
            UNIT_ASSERT_VALUES_EQUAL(foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo->GetState());
            UNIT_ASSERT_VALUES_EQUAL(2, foo->DevicesSize());

            auto& x = foo->GetDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("x", x.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, x.GetState());
            UNIT_ASSERT(x.GetStateMessage().empty());

            auto& y = foo->GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("y", y.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, y.GetState());
            UNIT_ASSERT(y.GetStateMessage().empty());
        }

        {
            auto r = agentList.RegisterAgent([] {
                    NProto::TAgentConfig foo;

                    foo.SetAgentId("foo");
                    foo.SetNodeId(1000);
                    *foo.AddDevices() = CreateDevice("x");
                    *foo.AddDevices() = CreateDevice("z");

                    return foo;
                }(),
                TInstant::FromValue(42),
                knownAgent);

            NProto::TAgentConfig& foo = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(1, r.NewDevices.size());
            UNIT_ASSERT_VALUES_EQUAL("z", *r.NewDevices.begin());
            UNIT_ASSERT_VALUES_EQUAL(&foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo.GetState());
            UNIT_ASSERT_VALUES_EQUAL(3, foo.DevicesSize());

            auto& x = *foo.MutableDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("x", x.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, x.GetState());
            UNIT_ASSERT(x.GetStateMessage().empty());
            x.SetState(NProto::DEVICE_STATE_WARNING);

            auto& y = foo.GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("y", y.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());
            UNIT_ASSERT(!y.GetStateMessage().empty());

            auto& z = foo.GetDevices(2);
            UNIT_ASSERT_VALUES_EQUAL("z", z.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, z.GetState());
            UNIT_ASSERT(z.GetStateMessage().empty());
        }

        {
            auto r = agentList.RegisterAgent([] {
                    NProto::TAgentConfig foo;

                    foo.SetAgentId("foo");
                    foo.SetNodeId(1000);
                    *foo.AddDevices() = CreateDevice("x");
                    *foo.AddDevices() = CreateDevice("y");
                    *foo.AddDevices() = CreateDevice("z");

                    return foo;
                }(),
                TInstant::FromValue(42),
                knownAgent);

            NProto::TAgentConfig& foo = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(0, r.NewDevices.size());
            UNIT_ASSERT_VALUES_EQUAL(&foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo.GetState());
            UNIT_ASSERT_VALUES_EQUAL(3, foo.DevicesSize());

            auto& x = foo.GetDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("x", x.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_WARNING, x.GetState());
            UNIT_ASSERT(x.GetStateMessage().empty());

            auto& y = foo.GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("y", y.GetDeviceUUID());
            // can't change state to online automatically
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());
            UNIT_ASSERT(!y.GetStateMessage().empty());

            auto& z = foo.GetDevices(2);
            UNIT_ASSERT_VALUES_EQUAL("z", z.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, z.GetState());
            UNIT_ASSERT(z.GetStateMessage().empty());
        }

        {
            auto r = agentList.RegisterAgent([] {
                    NProto::TAgentConfig foo;

                    foo.SetAgentId("foo");
                    foo.SetNodeId(1000);
                    auto& z = *foo.AddDevices();
                    z = CreateDevice("z");
                    z.SetBlockSize(512);

                    return foo;
                }(),
                TInstant::FromValue(42),
                knownAgent);

            NProto::TAgentConfig& foo = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(0, r.NewDevices.size());
            UNIT_ASSERT_VALUES_EQUAL(&foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo.GetState());
            UNIT_ASSERT_VALUES_EQUAL(3, foo.DevicesSize());

            auto& x = foo.GetDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("x", x.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, x.GetState());
            UNIT_ASSERT(!x.GetStateMessage().empty());

            auto& y = foo.GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("y", y.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());
            UNIT_ASSERT(!y.GetStateMessage().empty());

            auto& z = foo.GetDevices(2);
            UNIT_ASSERT_VALUES_EQUAL("z", z.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, z.GetState());
            UNIT_ASSERT(!z.GetStateMessage().empty());
        }
    }

    Y_UNIT_TEST_F(ShouldUpdateCounters, TFixture)
    {
        NProto::TAgentConfig foo;

        foo.SetAgentId("foo");
        foo.SetNodeId(1000);
        *foo.AddDevices() = CreateDevice("uuid-1");
        *foo.AddDevices() = CreateDevice("uuid-2");

        NProto::TAgentConfig bar;

        bar.SetAgentId("bar");
        bar.SetNodeId(2000);
        *bar.AddDevices() = CreateDevice("uuid-3");
        *bar.AddDevices() = CreateDevice("uuid-4");

        TAgentList agentList = CreateAgentList({
            .Agents = {foo, bar}
        });

        auto fooCounters = Counters->GetSubgroup("agent", foo.GetAgentId());
        auto barCounters = Counters->GetSubgroup("agent", bar.GetAgentId());

        auto uuid1Counters = fooCounters->GetSubgroup("device", "uuid-1");
        auto uuid2Counters = fooCounters->GetSubgroup("device", "uuid-2");

        auto uuid3Counters = barCounters->GetSubgroup("device", "uuid-3");
        auto uuid4Counters = barCounters->GetSubgroup("device", "uuid-4");

        auto uuid1ReadCount = uuid1Counters->GetCounter("ReadCount");
        auto uuid1WriteCount = uuid1Counters->GetCounter("WriteCount");

        auto uuid2ReadCount = uuid2Counters->GetCounter("ReadCount");
        auto uuid2WriteCount = uuid2Counters->GetCounter("WriteCount");

        auto uuid3ReadCount = uuid3Counters->GetCounter("ReadCount");
        auto uuid3WriteCount = uuid3Counters->GetCounter("WriteCount");

        auto uuid4ReadCount = uuid4Counters->GetCounter("ReadCount");
        auto uuid4WriteCount = uuid4Counters->GetCounter("WriteCount");

        UNIT_ASSERT_VALUES_EQUAL(0, uuid1ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, uuid1WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(0, uuid2ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, uuid2WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(0, uuid3ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, uuid3WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(0, uuid4ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, uuid4WriteCount->Val());

        agentList.UpdateCounters([] {
            NProto::TAgentStats stats;

            stats.SetNodeId(1000);

            auto* uuid1Stats = stats.AddDeviceStats();
            uuid1Stats->SetDeviceUUID("uuid-1");
            uuid1Stats->SetNumReadOps(10);
            uuid1Stats->SetNumWriteOps(20);

            auto* uuid2Stats = stats.AddDeviceStats();
            uuid2Stats->SetDeviceUUID("uuid-2");
            uuid2Stats->SetNumReadOps(30);
            uuid2Stats->SetNumWriteOps(40);

            return stats;
        }(), {});

        agentList.UpdateCounters([] {
            NProto::TAgentStats stats;

            stats.SetNodeId(2000);

            auto* uuid3Stats = stats.AddDeviceStats();
            uuid3Stats->SetDeviceUUID("uuid-3");
            uuid3Stats->SetNumReadOps(100);
            uuid3Stats->SetNumWriteOps(200);

            auto* uuid4Stats = stats.AddDeviceStats();
            uuid4Stats->SetDeviceUUID("uuid-4");
            uuid4Stats->SetNumReadOps(300);
            uuid4Stats->SetNumWriteOps(400);

            return stats;
        }(), {});

        agentList.PublishCounters(TInstant::Hours(1));

        UNIT_ASSERT_VALUES_EQUAL(10, uuid1ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(20, uuid1WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(30, uuid2ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(40, uuid2WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(100, uuid3ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(200, uuid3WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(300, uuid4ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(400, uuid4WriteCount->Val());

        agentList.RemoveAgent(1000);

        agentList.UpdateCounters([] {
            NProto::TAgentStats stats;

            stats.SetNodeId(1000);

            auto* uuid1Stats = stats.AddDeviceStats();
            uuid1Stats->SetDeviceUUID("uuid-1");
            uuid1Stats->SetNumReadOps(1000);
            uuid1Stats->SetNumWriteOps(1000);

            auto* uuid2Stats = stats.AddDeviceStats();
            uuid2Stats->SetDeviceUUID("uuid-2");
            uuid2Stats->SetNumReadOps(1000);
            uuid2Stats->SetNumWriteOps(1000);

            return stats;
        }(), {});

        agentList.UpdateCounters([] {
            NProto::TAgentStats stats;

            stats.SetNodeId(2000);

            auto* uuid3Stats = stats.AddDeviceStats();
            uuid3Stats->SetDeviceUUID("uuid-3");
            uuid3Stats->SetNumReadOps(1000);
            uuid3Stats->SetNumWriteOps(1000);

            auto* uuid4Stats = stats.AddDeviceStats();
            uuid4Stats->SetDeviceUUID("uuid-4");
            uuid4Stats->SetNumReadOps(1000);
            uuid4Stats->SetNumWriteOps(1000);

            return stats;
        }(), {});

        agentList.PublishCounters(TInstant::Hours(2));

        UNIT_ASSERT_VALUES_EQUAL(10, uuid1ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(20, uuid1WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(30, uuid2ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(40, uuid2WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(1100, uuid3ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(1200, uuid3WriteCount->Val());

        UNIT_ASSERT_VALUES_EQUAL(1300, uuid4ReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(1400, uuid4WriteCount->Val());
    }

    Y_UNIT_TEST_F(ShouldUpdateSeqNumber, TFixture)
    {
        TAgentList agentList = CreateAgentList({
            .Agents = [] {
                NProto::TAgentConfig foo;

                foo.SetAgentId("foo");
                foo.SetNodeId(1000);
                foo.SetSeqNumber(23);
                foo.SetDedicatedDiskAgent(false);
                *foo.AddDevices() = CreateDevice("uuid-1");
                *foo.AddDevices() = CreateDevice("uuid-2");

                return TVector{foo};
            }()
        });

        const TKnownAgent knownAgent {
            .Devices = {
                { "uuid-1", CreateDevice("uuid-1", 0) },
                { "uuid-2", CreateDevice("uuid-2", 0) },
            }};

        {
            auto* foo = agentList.FindAgent("foo");
            UNIT_ASSERT_VALUES_UNEQUAL(nullptr, foo);
            UNIT_ASSERT_VALUES_EQUAL(foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo->GetState());
            UNIT_ASSERT_VALUES_EQUAL(23, foo->GetSeqNumber());
            UNIT_ASSERT(!foo->GetDedicatedDiskAgent());
            UNIT_ASSERT_VALUES_EQUAL(2, foo->DevicesSize());
        }

        {
            auto r = agentList.RegisterAgent([] {
                    NProto::TAgentConfig foo;

                    foo.SetAgentId("foo");
                    foo.SetNodeId(1000);
                    foo.SetSeqNumber(27);
                    foo.SetDedicatedDiskAgent(true);
                    *foo.AddDevices() = CreateDevice("uuid-1");
                    *foo.AddDevices() = CreateDevice("uuid-2");

                    return foo;
                }(),
                TInstant::FromValue(42),
                knownAgent);

            NProto::TAgentConfig& foo = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(&foo, agentList.FindAgent(1000));
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, foo.GetState());
            UNIT_ASSERT_VALUES_EQUAL(27, foo.GetSeqNumber());
            UNIT_ASSERT(foo.GetDedicatedDiskAgent());
            UNIT_ASSERT_VALUES_EQUAL(2, foo.DevicesSize());
        }
    }

    Y_UNIT_TEST_F(ShouldCalculateRejectAgentTimeout, TFixture)
    {
        auto c = Counters->GetCounter("RejectAgentTimeout");

        TAgentList agentList = CreateAgentList({
            .Config = {
                .TimeoutGrowthFactor = 2,
                .MinRejectAgentTimeout = 30s,
                .MaxRejectAgentTimeout = 5min,
                .DisconnectRecoveryInterval = 1min,
            }
        });

        TInstant now = Now();

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(30s),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(30'000, c->Val());

        now += 1h;

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(30s),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(30'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(1min),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(60'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(2min),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(120'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(4min),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(240'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(5min),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(300'000, c->Val());

        now += 1min;

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(4min),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(240'000, c->Val());

        now += 1min;

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(2min),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(120'000, c->Val());

        now += 1min;

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(1min),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(60'000, c->Val());

        now += 1min;

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(30s),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(30'000, c->Val());

        now += 1min;

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(30s),
            agentList.GetRejectAgentTimeout(now, ""));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(30'000, c->Val());
    }

    Y_UNIT_TEST_F(ShouldCorrectlyAccumulateRejectAgentTimeoutMultiplier, TFixture)
    {
        TAgentList agentList = CreateAgentList({
            .Config = {
                .TimeoutGrowthFactor = 2,
                .MinRejectAgentTimeout = 30s,
                .MaxRejectAgentTimeout = 5min,
                .DisconnectRecoveryInterval = 1min,
            }
        });

        TInstant now = Now();

        agentList.OnAgentDisconnected(now);

        now += 55s;

        agentList.OnAgentDisconnected(now);

        now += 55s;

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(67'347'722us),
            agentList.GetRejectAgentTimeout(now, ""));
    }

    Y_UNIT_TEST_F(ShouldNotOverflowRejectAgentTimeout, TFixture)
    {
        const TDuration expectedMaxRejectAgentTimeout = 5min;

        TAgentList agentList = CreateAgentList({
            .Config = {
                .TimeoutGrowthFactor = 2,
                .MinRejectAgentTimeout = 30s,
                .MaxRejectAgentTimeout = expectedMaxRejectAgentTimeout,
                .DisconnectRecoveryInterval = 1min,
            }
        });

        TInstant now = Now();

        for (ui32 i = 0; i < 100; ++i) {
            agentList.OnAgentDisconnected(now);
        }

        UNIT_ASSERT_VALUES_EQUAL(
            expectedMaxRejectAgentTimeout,
            agentList.GetRejectAgentTimeout(now, ""));

        now += 2min;
        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(150s),
            agentList.GetRejectAgentTimeout(now, ""));
    }

    Y_UNIT_TEST_F(ShouldValidateSerialNumbers, TFixture)
    {
        TAgentList agentList = CreateAgentList({
            .Config = { .SerialNumberValidationEnabled = true }
        });

        const NProto::TAgentConfig expectedConfig = [] {
            NProto::TAgentConfig config;

            config.SetAgentId("foo");
            config.SetNodeId(1000);

            auto& dev0 = *config.AddDevices() = CreateDevice("uuid-1");
            dev0.SetSerialNumber("SN-1");

            auto& dev1 = *config.AddDevices() = CreateDevice("uuid-2");
            dev1.SetSerialNumber("UNK");

            auto& dev2 = *config.AddDevices() = CreateDevice("uuid-3");
            dev2.SetSerialNumber("SN-3");

            return config;
        }();

        TKnownAgent knownAgent;
        {
            auto& x = knownAgent.Devices["uuid-1"];
            x.SetDeviceUUID("uuid-1");
            x.SetSerialNumber("SN-1");

            auto& y = knownAgent.Devices["uuid-2"];
            y.SetDeviceUUID("uuid-2");
            y.SetSerialNumber("SN-2");

            auto& z = knownAgent.Devices["uuid-3"];
            z.SetDeviceUUID("uuid-3");
            z.SetSerialNumber("SN-3");
        }

        {
            auto r = agentList.RegisterAgent(
                expectedConfig,
                TInstant::FromValue(1),
                knownAgent);

            NProto::TAgentConfig& agent = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(agent.GetAgentId(), expectedConfig.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(agent.GetNodeId(), expectedConfig.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(agent.DevicesSize(), expectedConfig.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(3, r.NewDevices.size());

            agentList.PublishCounters(TInstant::Minutes(1));

            auto& x = agent.GetDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", x.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("SN-1", x.GetSerialNumber());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, x.GetState());

            auto& y = agent.GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", y.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("UNK", y.GetSerialNumber());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());

            auto& z = agent.GetDevices(2);
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", z.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("SN-3", z.GetSerialNumber());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, z.GetState());
        }

        knownAgent.Devices["uuid-3"].SetSerialNumber("XXX");

        {
            auto r = agentList.RegisterAgent(
                expectedConfig,
                TInstant::FromValue(1),
                knownAgent);

            NProto::TAgentConfig& agent = r.Agent;

            UNIT_ASSERT_VALUES_EQUAL(agent.GetAgentId(), expectedConfig.GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(agent.GetNodeId(), expectedConfig.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL(agent.DevicesSize(), expectedConfig.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, r.NewDevices.size());

            auto& x = agent.GetDevices(0);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", x.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("SN-1", x.GetSerialNumber());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, x.GetState());

            auto& y = agent.GetDevices(1);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", y.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("UNK", y.GetSerialNumber());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());

            auto& z = agent.GetDevices(2);
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", z.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("SN-3", z.GetSerialNumber());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, y.GetState());
        }
    }

    Y_UNIT_TEST_F(ShouldCalculateRejectAgentTimeoutWithUpdatedParams, TFixture)
    {
        auto c = Counters->GetCounter("RejectAgentTimeout");

        TAgentList agentList = CreateAgentList({
            .Config = {
                .TimeoutGrowthFactor = 2,
                .MinRejectAgentTimeout = 30s,
                .MaxRejectAgentTimeout = 5min,
                .DisconnectRecoveryInterval = 1min,
            }
        });

        NProto::TDiskRegistryAgentParams params;
        params.SetNewNonReplicatedAgentMinTimeoutMs(60 * 1000);
        params.SetNewNonReplicatedAgentMaxTimeoutMs(10 * 60 * 1000);
        agentList.SetDiskRegistryAgentListParams("agent-1", params);

        TInstant now = Now();

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(30s),
            agentList.GetRejectAgentTimeout(now, ""));

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(1min),
            agentList.GetRejectAgentTimeout(now, "agent-1"));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(30'000, c->Val());

        now += 1h;

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(30s),
            agentList.GetRejectAgentTimeout(now, ""));

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(1min),
            agentList.GetRejectAgentTimeout(now, "agent-1"));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(30'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(1min),
            agentList.GetRejectAgentTimeout(now, ""));

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(2min),
            agentList.GetRejectAgentTimeout(now, "agent-1"));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(60'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(2min),
            agentList.GetRejectAgentTimeout(now, ""));

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(4min),
            agentList.GetRejectAgentTimeout(now, "agent-1"));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(120'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(4min),
            agentList.GetRejectAgentTimeout(now, ""));

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(8min),
            agentList.GetRejectAgentTimeout(now, "agent-1"));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(240'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(5min),
            agentList.GetRejectAgentTimeout(now, ""));

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(10min),
            agentList.GetRejectAgentTimeout(now, "agent-1"));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(300'000, c->Val());

        agentList.OnAgentDisconnected(now);

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(5min),
            agentList.GetRejectAgentTimeout(now, ""));

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(10min),
            agentList.GetRejectAgentTimeout(now, "agent-1"));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(300'000, c->Val());

        agentList.OnAgentDisconnected(now);

        now += 1h;

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(30s),
            agentList.GetRejectAgentTimeout(now, ""));

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration(1min),
            agentList.GetRejectAgentTimeout(now, "agent-1"));

        agentList.PublishCounters(now);
        UNIT_ASSERT_VALUES_EQUAL(30'000, c->Val());
    }
}

}   // namespace NCloud::NBlockStore::NStorage
