#include <ydb/core/cms/console/configs_dispatcher.h>
#include <ydb/core/testlib/cs_helper.h>
#include <ydb/core/tx/tiering/external_data.h>
#include <ydb/core/tx/schemeshard/schemeshard.h>
#include <ydb/core/tx/tx_proxy/proxy.h>
#include <ydb/core/wrappers/ut_helpers/s3_mock.h>
#include <ydb/core/wrappers/s3_wrapper.h>
#include <ydb/core/wrappers/fake_storage.h>
#include <ydb/library/accessor/accessor.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>
#include <ydb/services/metadata/manager/alter.h>
#include <ydb/services/metadata/manager/common.h>
#include <ydb/services/metadata/manager/table_record.h>
#include <ydb/services/metadata/manager/ydb_value_operator.h>
#include <ydb/services/metadata/service.h>

#include <library/cpp/actors/core/av_bootstrapped.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/system/hostname.h>

namespace NKikimr {

using namespace NColumnShard;

class TLocalHelper: public Tests::NCS::THelper {
private:
    using TBase = Tests::NCS::THelper;
public:
    using TBase::TBase;
    void CreateTestOlapTable(TString tableName = "olapTable", ui32 tableShardsCount = 3,
        TString storeName = "olapStore", ui32 storeShardsCount = 4,
        TString shardingFunction = "HASH_FUNCTION_CLOUD_LOGS") {
        TActorId sender = Server.GetRuntime()->AllocateEdgeActor();
        CreateTestOlapStore(sender, Sprintf(R"(
             Name: "%s"
             ColumnShardCount: %d
             SchemaPresets {
                 Name: "default"
                 Schema {
                     %s
                 }
             }
        )", storeName.c_str(), storeShardsCount, GetTestTableSchema().data()));

        TString shardingColumns = "[\"timestamp\", \"uid\"]";
        if (shardingFunction != "HASH_FUNCTION_CLOUD_LOGS") {
            shardingColumns = "[\"uid\"]";
        }

        TBase::CreateTestOlapTable(sender, storeName, Sprintf(R"(
            Name: "%s"
            ColumnShardCount: %d
            TtlSettings: {
                UseTiering: "tiering1"
            }
            Sharding {
                HashSharding {
                    Function: %s
                    Columns: %s
                }
            }
        )", tableName.c_str(), tableShardsCount, shardingFunction.c_str(), shardingColumns.c_str()));
    }

    void CreateTestOlapTableWithTTL(TString tableName = "olapTable", ui32 tableShardsCount = 3,
        TString storeName = "olapStore", ui32 storeShardsCount = 4,
        TString shardingFunction = "HASH_FUNCTION_CLOUD_LOGS") {

        TActorId sender = Server.GetRuntime()->AllocateEdgeActor();
        CreateTestOlapStore(sender, Sprintf(R"(
             Name: "%s"
             ColumnShardCount: %d
             SchemaPresets {
                 Name: "default"
                 Schema {
                     %s
                 }
             }
        )", storeName.c_str(), storeShardsCount, GetTestTableSchema().data()));

        TString shardingColumns = "[\"timestamp\", \"uid\"]";
        if (shardingFunction != "HASH_FUNCTION_CLOUD_LOGS") {
            shardingColumns = "[\"uid\"]";
        }

        TBase::CreateTestOlapTable(sender, storeName, Sprintf(R"(
            Name: "%s"
            ColumnShardCount: %d
            TtlSettings: {
                Enabled: {
                    ColumnName : "timestamp"
                    ExpireAfterSeconds : 86400
                }
            }
            Sharding {
                HashSharding {
                    Function: %s
                    Columns: %s
                }
            }
        )", tableName.c_str(), tableShardsCount, shardingFunction.c_str(), shardingColumns.c_str()));
    }
};


Y_UNIT_TEST_SUITE(ColumnShardTiers) {

    const TString ConfigProtoStr = "Name : \"abc\"";
    const TString ConfigProtoStr1 = "Name : \"abc1\"";
    const TString ConfigProtoStr2 = "Name : \"abc2\"";

    const TString ConfigTiering1Str = R"({
        "rules" : [
            {
                "tierName" : "tier1",
                "durationForEvict" : "10d"
            },
            {
                "tierName" : "tier2",
                "durationForEvict" : "20d"
            }
        ]
    })";

    const TString ConfigTiering2Str = R"({
        "rules" : [
            {
                "tierName" : "tier1",
                "durationForEvict" : "10d"
            }
        ]
    })";

    class TJsonChecker {
    private:
        YDB_ACCESSOR_DEF(TString, Path);
        YDB_ACCESSOR_DEF(TString, Expectation);
    public:
        TJsonChecker(const TString& path, const TString& expectation)
            : Path(path)
            , Expectation(expectation)
        {

        }
        bool Check(const NJson::TJsonValue& jsonInfo) const {
            auto* jsonPathValue = jsonInfo.GetValueByPath(Path);
            if (!jsonPathValue) {
                return Expectation == "__NULL";
            }
            return jsonPathValue->GetStringRobust() == Expectation;
        }

        TString GetDebugString() const {
            TStringBuilder sb;
            sb << "path=" << Path << ";"
                << "expectation=" << Expectation << ";";
            return sb;
        }
    };

    class TTestCSEmulator: public NActors::TActorBootstrapped<TTestCSEmulator> {
    private:
        using TBase = NActors::TActorBootstrapped<TTestCSEmulator>;
        std::shared_ptr<NTiers::TSnapshotConstructor> ExternalDataManipulation;
        TActorId ProviderId;
        TInstant Start;
        YDB_READONLY_FLAG(Found, false);
        YDB_ACCESSOR(ui32, ExpectedTieringsCount, 1);
        YDB_ACCESSOR(ui32, ExpectedTiersCount, 1);

        using TKeyCheckers = TMap<TString, TJsonChecker>;
        YDB_ACCESSOR_DEF(TKeyCheckers, Checkers);
    public:
        void ResetConditions() {
            FoundFlag = false;
            Checkers.clear();
        }

        STATEFN(StateInit) {
            switch (ev->GetTypeRewrite()) {
                hFunc(NMetadata::NProvider::TEvRefreshSubscriberData, Handle);
                default:
                    Y_VERIFY(false);
            }
        }

        void CheckRuntime(TTestActorRuntime& runtime) {
            const auto pred = [this](TTestActorRuntimeBase&, TAutoPtr<IEventHandle>& event)->TTestActorRuntimeBase::EEventAction {
                if (event->HasBuffer() && !event->HasEvent()) {
                } else if (!event->HasEvent()) {
                } else {
                    auto ptr = event->CastAsLocal<NMetadata::NProvider::TEvRefreshSubscriberData>();
                    if (ptr) {
                        CheckFound(ptr);
                    }
                }
                return TTestActorRuntimeBase::EEventAction::PROCESS;
            };

            runtime.SetObserverFunc(pred);

            for (const TInstant start = Now(); !IsFound() && Now() - start < TDuration::Seconds(30); ) {
                runtime.SimulateSleep(TDuration::Seconds(1));
            }
            runtime.SetObserverFunc(TTestActorRuntime::DefaultObserverFunc);
            Y_VERIFY(IsFound());
        }

        void CheckFound(NMetadata::NProvider::TEvRefreshSubscriberData* event) {
            auto snapshot = event->GetSnapshotAs<NTiers::TConfigsSnapshot>();
            if (!snapshot) {
                Cerr << "incorrect snapshot" << Endl;
                return;
            }
            Cerr << "SNAPSHOT: " << snapshot->SerializeToString() << Endl;
            const auto& tierings = snapshot->GetTableTierings();
            if (tierings.size() != ExpectedTieringsCount) {
                Cerr << "TieringsCount incorrect: " << snapshot->SerializeToString() << ";expectation=" << ExpectedTieringsCount << Endl;
                return;
            }
            if (ExpectedTiersCount != snapshot->GetTierConfigs().size()) {
                Cerr << "TiersCount incorrect: " << snapshot->SerializeToString() << ";expectation=" << ExpectedTiersCount << Endl;
                return;
            }
            for (auto&& i : Checkers) {
                NJson::TJsonValue jsonData;
                if (i.first.StartsWith("TIER.")) {
                    auto value = snapshot->GetTierById(i.first.substr(5));
                    jsonData = value->SerializeConfigToJson();
                } else if (i.first.StartsWith("TIERING_RULE.")) {
                    auto value = snapshot->GetTierById(i.first.substr(13));
                    jsonData = value->SerializeConfigToJson();
                } else {
                    Y_VERIFY(false);
                }
                if (!i.second.Check(jsonData)) {
                    Cerr << "config value incorrect:" << snapshot->SerializeToString() << ";snapshot_check_path=" << i.first << Endl;
                    Cerr << "json path incorrect:" << jsonData << ";" << i.second.GetDebugString() << Endl;
                    return;
                }
            }
            FoundFlag = true;
        }

        void Handle(NMetadata::NProvider::TEvRefreshSubscriberData::TPtr& ev) {
            CheckFound(ev->Get());
        }

        void Bootstrap() {
            ProviderId = NMetadata::NProvider::MakeServiceId(SelfId().NodeId());
            ExternalDataManipulation = std::make_shared<NTiers::TSnapshotConstructor>();
            Become(&TThis::StateInit);
            Sender<NMetadata::NProvider::TEvSubscribeExternal>(ExternalDataManipulation).SendTo(ProviderId);
            Start = Now();
        }
    };

    class TEmulatorAlterController: public NMetadata::NModifications::IAlterController {
    private:
        YDB_READONLY_FLAG(Finished, false);
    public:
        virtual void OnAlteringProblem(const TString& errorMessage) override {
            Cerr << errorMessage << Endl;
            Y_VERIFY(false);
        }
        virtual void OnAlteringFinished() override {
            FinishedFlag = true;
        }
    };

    Y_UNIT_TEST(DSConfigsStub) {
        TPortManager pm;

        ui32 grpcPort = pm.GetPort();
        ui32 msgbPort = pm.GetPort();

        Tests::TServerSettings serverSettings(msgbPort);
        serverSettings.Port = msgbPort;
        serverSettings.GrpcPort = grpcPort;
        serverSettings.SetDomainName("Root")
            .SetUseRealThreads(false)
            .SetEnableMetadataProvider(true)
            .SetForceColumnTablesCompositeMarks(true);
        ;

        Tests::TServer::TPtr server = new Tests::TServer(serverSettings);
        server->EnableGRpc(grpcPort);
        //        server->SetupDefaultProfiles();

        Tests::TClient client(serverSettings);

        auto& runtime = *server->GetRuntime();

        auto sender = runtime.AllocateEdgeActor();
        server->SetupRootStoragePools(sender);
        TLocalHelper lHelper(*server);
        lHelper.CreateTestOlapTable();
        {
            TTestCSEmulator* emulator = new TTestCSEmulator;
            emulator->MutableCheckers().emplace("TIER.tier1", TJsonChecker("Name", "abc"));
            emulator->SetExpectedTiersCount(2);
            runtime.Register(emulator);
            runtime.SimulateSleep(TDuration::Seconds(10));
            Cerr << "Initialization finished" << Endl;

            lHelper.StartSchemaRequest("CREATE OBJECT tier1 (TYPE TIER) WITH tierConfig = `" + ConfigProtoStr + "`");
            lHelper.StartSchemaRequest("CREATE OBJECT tiering1 ("
                "TYPE TIERING_RULE) WITH (defaultColumn = timestamp, description = `" + ConfigTiering1Str + "` )", false);
            lHelper.StartSchemaRequest("CREATE OBJECT tier2 (TYPE TIER) WITH tierConfig = `" + ConfigProtoStr + "`");
            lHelper.StartSchemaRequest("CREATE OBJECT tiering1 ("
                "TYPE TIERING_RULE) WITH (defaultColumn = timestamp, description = `" + ConfigTiering1Str + "` )");
            {
                const TInstant start = Now();
                while (!emulator->IsFound() && Now() - start < TDuration::Seconds(2000)) {
                    runtime.SimulateSleep(TDuration::Seconds(1));
                }
                Y_VERIFY(emulator->IsFound());
            }
            {
                emulator->ResetConditions();
                emulator->SetExpectedTiersCount(2);
                emulator->MutableCheckers().emplace("TIER.tier1", TJsonChecker("Name", "abc1"));

                lHelper.StartSchemaRequest("ALTER OBJECT tier1 (TYPE TIER) SET tierConfig = `" + ConfigProtoStr1 + "`");

                {
                    const TInstant start = Now();
                    while (!emulator->IsFound() && Now() - start < TDuration::Seconds(2000)) {
                        runtime.SimulateSleep(TDuration::Seconds(1));
                    }
                    Y_VERIFY(emulator->IsFound());
                }
            }
            {
                emulator->ResetConditions();
                emulator->SetExpectedTieringsCount(0);
                emulator->SetExpectedTiersCount(0);

                lHelper.StartSchemaRequest("DROP OBJECT tier1(TYPE TIER)", false);
                lHelper.StartSchemaRequest("DROP OBJECT tiering1(TYPE TIERING_RULE)", false);
                lHelper.StartSchemaRequest("DROP TABLE `/Root/olapStore/olapTable`");
                lHelper.StartSchemaRequest("DROP OBJECT tiering1(TYPE TIERING_RULE)");
                lHelper.StartSchemaRequest("DROP OBJECT tier1(TYPE TIER)");
                lHelper.StartSchemaRequest("DROP OBJECT tier2(TYPE TIER)");

                {
                    const TInstant start = Now();
                    while (!emulator->IsFound() && Now() - start < TDuration::Seconds(20)) {
                        runtime.SimulateSleep(TDuration::Seconds(1));
                    }
                    Y_VERIFY(emulator->IsFound());
                }
            }
        }
    }

    Y_UNIT_TEST(DSConfigs) {
        TPortManager pm;

        ui32 grpcPort = pm.GetPort();
        ui32 msgbPort = pm.GetPort();

        Tests::TServerSettings serverSettings(msgbPort);
        serverSettings.Port = msgbPort;
        serverSettings.GrpcPort = grpcPort;
        serverSettings.SetDomainName("Root")
            .SetUseRealThreads(false)
            .SetEnableMetadataProvider(true)
            .SetForceColumnTablesCompositeMarks(true);

        Tests::TServer::TPtr server = new Tests::TServer(serverSettings);
        server->EnableGRpc(grpcPort);
        Tests::TClient client(serverSettings);

        auto& runtime = *server->GetRuntime();

        auto sender = runtime.AllocateEdgeActor();
        server->SetupRootStoragePools(sender);
        TLocalHelper lHelper(*server);
        lHelper.CreateTestOlapTable("olapTable");

        runtime.SetLogPriority(NKikimrServices::TX_DATASHARD, NLog::PRI_NOTICE);
        runtime.SetLogPriority(NKikimrServices::TX_COLUMNSHARD, NLog::PRI_INFO);
        //        runtime.SetLogPriority(NKikimrServices::TX_PROXY_SCHEME_CACHE, NLog::PRI_DEBUG);
        runtime.SimulateSleep(TDuration::Seconds(10));
        Cerr << "Initialization finished" << Endl;

        lHelper.StartSchemaRequest("CREATE OBJECT tier1 (TYPE TIER) WITH tierConfig = `" + ConfigProtoStr1 + "`", true, false);
        {
            TTestCSEmulator emulator;
            emulator.MutableCheckers().emplace("TIER.tier1", TJsonChecker("Name", "abc1"));
            emulator.SetExpectedTieringsCount(0);
            emulator.SetExpectedTiersCount(1);
            emulator.CheckRuntime(runtime);
        }

        lHelper.StartSchemaRequest("CREATE OBJECT tier2 (TYPE TIER) WITH tierConfig = `" + ConfigProtoStr2 + "`");
        lHelper.StartSchemaRequest("CREATE OBJECT tiering1 (TYPE TIERING_RULE) "
            "WITH (defaultColumn = timestamp, description = `" + ConfigTiering1Str + "`)");
        lHelper.StartSchemaRequest("CREATE OBJECT tiering2 (TYPE TIERING_RULE) "
            "WITH (defaultColumn = timestamp, description = `" + ConfigTiering2Str + "` )", true, false);
        {
            TTestCSEmulator emulator;
            emulator.MutableCheckers().emplace("TIER.tier1", TJsonChecker("Name", "abc1"));
            emulator.MutableCheckers().emplace("TIER.tier2", TJsonChecker("Name", "abc2"));
            emulator.SetExpectedTieringsCount(2);
            emulator.SetExpectedTiersCount(2);
            emulator.CheckRuntime(runtime);
        }

        lHelper.StartSchemaRequest("DROP OBJECT tier2 (TYPE TIER)", false);
        lHelper.StartSchemaRequest("DROP OBJECT tier1 (TYPE TIER)", false);
        lHelper.StartSchemaRequest("DROP OBJECT tiering2 (TYPE TIERING_RULE)");
        lHelper.StartSchemaRequest("DROP OBJECT tiering1 (TYPE TIERING_RULE)", false);
        lHelper.StartSchemaRequest("DROP TABLE `/Root/olapStore/olapTable`");
        lHelper.StartSchemaRequest("DROP OBJECT tiering1 (TYPE TIERING_RULE)", true, false);
        {
            TTestCSEmulator emulator;
            emulator.SetExpectedTieringsCount(0);
            emulator.SetExpectedTiersCount(2);
            emulator.CheckRuntime(runtime);
        }
        lHelper.StartSchemaRequest("DROP OBJECT tier2 (TYPE TIER)");
        lHelper.StartSchemaRequest("DROP OBJECT tier1 (TYPE TIER)", true, false);
        {
            TTestCSEmulator emulator;
            emulator.SetExpectedTieringsCount(0);
            emulator.SetExpectedTiersCount(0);
            emulator.CheckRuntime(runtime);
        }

        //runtime.SetLogPriority(NKikimrServices::TX_PROXY, NLog::PRI_TRACE);
        //runtime.SetLogPriority(NKikimrServices::KQP_YQL, NLog::PRI_TRACE);
    }
//#define S3_TEST_USAGE
#ifdef S3_TEST_USAGE
    const TString TierConfigProtoStr =
        R"(
        Name : "fakeTier"
        ObjectStorage : {
            Scheme: HTTP
            VerifySSL: false
            Endpoint: "storage.cloud-preprod.yandex.net"
            Bucket: "tiering-test-01"
            AccessKey: "SId:secretAccessKey"
            SecretKey: "USId:root@builtin:secretSecretKey"
            ProxyHost: "localhost"
            ProxyPort: 8080
            ProxyScheme: HTTP
        }
    )";
    const TString TierEndpoint = "storage.cloud-preprod.yandex.net";
#else
    const TString TierConfigProtoStr =
        R"(
        Name : "fakeTier"
        ObjectStorage : {
            Endpoint: "fake"
            Bucket: "fake"
            SecretableAccessKey: {
                SecretId: {
                    Id: "secretAccessKey"
                    OwnerId: "root@builtin"
                }
            }
            SecretKey: "SId:secretSecretKey"
        }
    )";
    const TString TierEndpoint = "fake";
#endif

    Y_UNIT_TEST(TieringUsage) {
        TPortManager pm;

        ui32 grpcPort = pm.GetPort();
        ui32 msgbPort = pm.GetPort();

        Tests::TServerSettings serverSettings(msgbPort);
        serverSettings.Port = msgbPort;
        serverSettings.GrpcPort = grpcPort;
        serverSettings.SetDomainName("Root")
            .SetUseRealThreads(false)
            .SetEnableMetadataProvider(true)
            .SetEnableBackgroundTasks(true)
            .SetForceColumnTablesCompositeMarks(true);
        ;

        Tests::TServer::TPtr server = new Tests::TServer(serverSettings);
        server->EnableGRpc(grpcPort);
        Tests::TClient client(serverSettings);
        Tests::NCommon::TLoggerInit(server->GetRuntime()).SetComponents({ NKikimrServices::TX_COLUMNSHARD }).Initialize();

        auto& runtime = *server->GetRuntime();
//        runtime.SetLogPriority(NKikimrServices::TX_PROXY, NLog::PRI_TRACE);
//        runtime.SetLogPriority(NKikimrServices::KQP_YQL, NLog::PRI_TRACE);

        auto sender = runtime.AllocateEdgeActor();
        server->SetupRootStoragePools(sender);

//        runtime.SetLogPriority(NKikimrServices::TX_DATASHARD, NLog::PRI_NOTICE);
        runtime.SetLogPriority(NKikimrServices::TX_COLUMNSHARD, NLog::PRI_DEBUG);
        runtime.SetLogPriority(NKikimrServices::BG_TASKS, NLog::PRI_DEBUG);
        //        runtime.SetLogPriority(NKikimrServices::TX_PROXY_SCHEME_CACHE, NLog::PRI_DEBUG);

        TLocalHelper lHelper(*server);
        lHelper.StartSchemaRequest("CREATE OBJECT secretAccessKey ( "
            "TYPE SECRET) WITH (value = ak)");
        lHelper.StartSchemaRequest("CREATE OBJECT secretSecretKey ( "
            "TYPE SECRET) WITH (value = sk)");
        Singleton<NKikimr::NWrappers::NExternalStorage::TFakeExternalStorage>()->SetSecretKey("sk");

        lHelper.StartSchemaRequest("CREATE OBJECT tier1 ( "
            "TYPE TIER) WITH (tierConfig = `" + TierConfigProtoStr + "`)");
        lHelper.StartSchemaRequest("CREATE OBJECT tier2 ( "
            "TYPE TIER) WITH (tierConfig = `" + TierConfigProtoStr + "`)");

        lHelper.StartSchemaRequest("CREATE OBJECT tiering1 ("
            "TYPE TIERING_RULE) WITH (defaultColumn = timestamp, description = `" + ConfigTiering1Str + "` )");
        lHelper.StartSchemaRequest("CREATE OBJECT tiering2 ("
            "TYPE TIERING_RULE) WITH (defaultColumn = timestamp, description = `" + ConfigTiering2Str + "` )");
        {
            TTestCSEmulator* emulator = new TTestCSEmulator;
            runtime.Register(emulator);
            emulator->MutableCheckers().emplace("TIER.tier1", TJsonChecker("Name", "fakeTier"));
            emulator->MutableCheckers().emplace("TIER.tier2", TJsonChecker("ObjectStorage.Endpoint", TierEndpoint));
            emulator->SetExpectedTieringsCount(2);
            emulator->SetExpectedTiersCount(2);
            emulator->CheckRuntime(runtime);
        }
        lHelper.CreateTestOlapTable("olapTable", 2);
        Cerr << "Wait tables" << Endl;
        runtime.SimulateSleep(TDuration::Seconds(20));
        Cerr << "Initialization tables" << Endl;
        const TInstant pkStart = Now() - TDuration::Days(15);

        auto batch = lHelper.TestArrowBatch(0, pkStart.GetValue(), 6000);
        auto batchSize = NArrow::GetBatchDataSize(batch);
        Cerr << "Inserting " << batchSize << " bytes..." << Endl;
        UNIT_ASSERT(batchSize > 4 * 1024 * 1024); // NColumnShard::TLimits::MIN_BYTES_TO_INSERT
        UNIT_ASSERT(batchSize < 8 * 1024 * 1024);

        {
            TAtomic unusedPrev;
            runtime.GetAppData().Icb->SetValue("ColumnShardControls.GranuleIndexedPortionsCountLimit", 1, unusedPrev);
        }
        for (ui32 i = 0; i < 4; ++i) {
            lHelper.SendDataViaActorSystem("/Root/olapStore/olapTable", batch);
        }
        {
            const TInstant start = Now();
            bool check = false;
            while (Now() - start < TDuration::Seconds(60)) {
                Cerr << "Waiting..." << Endl;
#ifndef S3_TEST_USAGE
                if (Singleton<NKikimr::NWrappers::NExternalStorage::TFakeExternalStorage>()->GetSize()) {
                    check = true;
                    Cerr << "Fake storage filled" << Endl;
                    break;
                }
#else
                check = true;
#endif
                runtime.AdvanceCurrentTime(TDuration::Minutes(6));
                runtime.SimulateSleep(TDuration::Seconds(1));
            }
            UNIT_ASSERT(check);
        }
#ifdef S3_TEST_USAGE
        Cerr << "storage initialized..." << Endl;
#endif

        lHelper.DropTable("/Root/olapStore/olapTable");
        {
            const TInstant start = Now();
            bool check = false;
            while (Now() - start < TDuration::Seconds(60)) {
                Cerr << "Cleaning waiting..." << Endl;
#ifndef S3_TEST_USAGE
                if (!Singleton<NKikimr::NWrappers::NExternalStorage::TFakeExternalStorage>()->GetSize()) {
                    check = true;
                    Cerr << "Fake storage clean" << Endl;
                    break;
                }
#else
                check = true;
#endif
                runtime.AdvanceCurrentTime(TDuration::Minutes(6));
                runtime.SimulateSleep(TDuration::Seconds(1));
            }
            UNIT_ASSERT(check);
        }
#ifndef S3_TEST_USAGE
        UNIT_ASSERT_EQUAL(Singleton<NKikimr::NWrappers::NExternalStorage::TFakeExternalStorage>()->GetBucketsCount(), 1);
#endif
    }

    std::optional<NYdb::TValue> GetValueResult(const THashMap<TString, NYdb::TValue>& hMap, const TString& fName) {
        auto it = hMap.find(fName);
        if (it == hMap.end()) {
            Cerr << fName << ": NOT_FOUND" << Endl;
            return {};
        } else {
            Cerr << fName << ": " << it->second.GetProto().DebugString() << Endl;
            return it->second;
        }
    }

    class TGCSource {
    private:
        const ui64 TabletId;
        const ui32 Channel;
    public:
        ui32 GetChannel() const {
            return Channel;
        }
        TGCSource(const ui64 tabletId, const ui32 channel)
            : TabletId(tabletId)
            , Channel(channel)
        {

        }

        bool operator<(const TGCSource& item) const {
            return std::tie(TabletId, Channel) < std::tie(item.TabletId, item.Channel);
        }

        TString DebugString() const {
            return TStringBuilder() << "tId=" << TabletId << ";c=" << Channel << ";";
        }
    };

    class TCurrentBarrier {
    private:
        ui32 Generation = 0;
        ui32 Step = 0;
    public:
        TCurrentBarrier() = default;

        TCurrentBarrier(const ui32 gen, const ui32 step)
            : Generation(gen)
            , Step(step)
        {

        }

        bool operator<(const TCurrentBarrier& b) const {
            return std::tie(Generation, Step) < std::tie(b.Generation, b.Step);
        }

        bool IsDeprecated(const NKikimr::TLogoBlobID& id) const {
            if (id.Generation() < Generation) {
                return true;
            }
            if (id.Generation() > Generation) {
                return false;
            }

            if (id.Step() < Step) {
                return true;
            }
            return id.Generation() == Generation && id.Step() == Step;
        }
    };

    class TBlobFlags {
    private:
        bool KeepFlag = false;
        bool DontKeepFlag = false;
    public:
        bool IsRemovable() const {
            return !KeepFlag || DontKeepFlag;
        }
        void Keep() {
            KeepFlag = true;
        }
        void DontKeep() {
            DontKeepFlag = true;
        }
    };

    class TGCSourceData {
    private:
        i64 BytesSize = 0;
        TCurrentBarrier Barrier;
        std::map<NKikimr::TLogoBlobID, TBlobFlags> Blobs;
    public:

        TString DebugString() const {
            return TStringBuilder() << "size=" << BytesSize << ";count=" << Blobs.size() << ";";
        }

        i64 GetSize() const {
            return BytesSize;
        }
        void AddSize(const ui64 size) {
            BytesSize += size;
        }
        void ReduceSize(const ui64 size) {
            BytesSize -= size;
            Y_VERIFY(BytesSize >= 0);
        }
        void SetBarrier(const TCurrentBarrier& b) {
            Y_VERIFY(!(b < Barrier));
            Barrier = b;
            RefreshBarrier();
        }

        void AddKeep(const TLogoBlobID& id) {
            auto it = Blobs.find(id);
            if (it != Blobs.end()) {
                it->second.Keep();
            }
        }

        void AddDontKeep(const TLogoBlobID& id) {
            auto it = Blobs.find(id);
            if (it != Blobs.end()) {
                it->second.DontKeep();
            }
        }

        void AddBlob(const TLogoBlobID& id) {
            Blobs[id] = TBlobFlags();
        }

        void RefreshBarrier() {
            for (auto it = Blobs.begin(); it != Blobs.end();) {
                if (Barrier.IsDeprecated(it->first) && it->second.IsRemovable()) {
                    ReduceSize(it->first.BlobSize());
                    it = Blobs.erase(it);
                } else {
                    ++it;
                }
            }
        }
    };

    class TBSDataCollector {
    private:
        std::map<TGCSource, TGCSourceData> Data;
    public:
        TGCSourceData& GetData(const TGCSource& id) {
            return Data[id];
        }
        ui64 GetChannelSize(const ui32 channelId) const {
            ui64 result = 0;
            for (auto&& i : Data) {
                if (i.first.GetChannel() == channelId) {
                    result += i.second.GetSize();
                }
            }
            return result;
        }
        ui64 GetSize() const {
            ui64 result = 0;
            for (auto&& i : Data) {
                result += i.second.GetSize();
            }
            return result;
        }
        TString StatusString() const {
            std::map<ui32, TString> info;
            for (auto&& i : Data) {
                info[i.first.GetChannel()] += i.second.DebugString();
            }
            TStringBuilder sb;
            for (auto&& i : info) {
                sb << i.first << ":" << i.second << ";";
            }
            return sb;
        }

    };

    Y_UNIT_TEST(TTLUsage) {
        TPortManager pm;

        ui32 grpcPort = pm.GetPort();
        ui32 msgbPort = pm.GetPort();

        Tests::TServerSettings serverSettings(msgbPort);
        serverSettings.Port = msgbPort;
        serverSettings.GrpcPort = grpcPort;
        serverSettings.SetDomainName("Root")
            .SetUseRealThreads(false)
            .SetEnableMetadataProvider(true)
            .SetEnableBackgroundTasks(true)
            .SetForceColumnTablesCompositeMarks(true);
        ;

        Tests::TServer::TPtr server = new Tests::TServer(serverSettings);
        server->EnableGRpc(grpcPort);
        Tests::TClient client(serverSettings);

        auto& runtime = *server->GetRuntime();
//        runtime.SetLogPriority(NKikimrServices::TX_PROXY, NLog::PRI_TRACE);
//        runtime.SetLogPriority(NKikimrServices::KQP_YQL, NLog::PRI_TRACE);

        auto sender = runtime.AllocateEdgeActor();
        server->SetupRootStoragePools(sender);

//        runtime.SetLogPriority(NKikimrServices::TX_DATASHARD, NLog::PRI_NOTICE);
        runtime.SetLogPriority(NKikimrServices::TX_COLUMNSHARD, NLog::PRI_DEBUG);
//        runtime.SetLogPriority(NKikimrServices::BG_TASKS, NLog::PRI_DEBUG);
        //        runtime.SetLogPriority(NKikimrServices::TX_PROXY_SCHEME_CACHE, NLog::PRI_DEBUG);

        TLocalHelper lHelper(*server);
        lHelper.CreateTestOlapTableWithTTL("olapTable", 1);
        Cerr << "Wait tables" << Endl;
        runtime.SimulateSleep(TDuration::Seconds(20));
        Cerr << "Initialization tables" << Endl;
        const ui32 numRecords = 600000;
        auto batch = lHelper.TestArrowBatch(0, TInstant::Zero().GetValue(), 600000, 1000000);

        ui32 gcCounter = 0;
        TBSDataCollector bsCollector;
        auto captureEvents = [&](TTestActorRuntimeBase&, TAutoPtr<IEventHandle>& ev) {
            if (auto* msg = dynamic_cast<TEvBlobStorage::TEvCollectGarbageResult*>(ev->StaticCastAsLocal<IEventBase>())) {
                Y_VERIFY(msg->Status == NKikimrProto::EReplyStatus::OK);
            }
            if (auto* msg = dynamic_cast<TEvBlobStorage::TEvCollectGarbage*>(ev->StaticCastAsLocal<IEventBase>())) {
                TGCSource gcSource(msg->TabletId, msg->Channel);
                auto& gcSourceData = bsCollector.GetData(gcSource);
                if (msg->Keep) {
                    for (auto&& i : *msg->Keep) {
                        gcSourceData.AddKeep(i);
                    }
                }
                if (msg->DoNotKeep) {
                    for (auto&& i : *msg->DoNotKeep) {
                        gcSourceData.AddDontKeep(i);
                    }
                }

                Y_VERIFY(!msg->Hard);
                if (msg->Collect) {
                    gcSourceData.SetBarrier(TCurrentBarrier(msg->CollectGeneration, msg->CollectStep));
                } else {
                    gcSourceData.RefreshBarrier();
                }
                Cerr << "TEvBlobStorage::TEvCollectGarbage " << gcSource.DebugString() << ":" << ++gcCounter << "/" << bsCollector.StatusString() << Endl;
            }
            if (auto* msg = dynamic_cast<TEvBlobStorage::TEvPut*>(ev->StaticCastAsLocal<IEventBase>())) {
                TGCSource gcSource(msg->Id.TabletID(), msg->Id.Channel());
                auto& gcSourceData = bsCollector.GetData(gcSource);
                gcSourceData.AddBlob(msg->Id);
                gcSourceData.AddSize(msg->Id.BlobSize());
                Cerr << "TEvBlobStorage::TEvPut " << gcSource.DebugString() << ":" << gcCounter << "/" << bsCollector.StatusString() << Endl;
            }
            return false;
        };
        runtime.SetEventFilter(captureEvents);

        lHelper.SendDataViaActorSystem("/Root/olapStore/olapTable", batch);

        {
            TVector<THashMap<TString, NYdb::TValue>> result;
            lHelper.StartScanRequest("SELECT MAX(timestamp) as a, MIN(timestamp) as b, COUNT(*) as c FROM `/Root/olapStore/olapTable`", true, &result);
            UNIT_ASSERT(result.size() == 1);
            UNIT_ASSERT(result.front().size() == 3);
            UNIT_ASSERT(GetValueResult(result.front(), "c")->GetProto().uint64_value() == 600000);
            UNIT_ASSERT(GetValueResult(result.front(), "a")->GetProto().uint64_value() == 599999000000);
            UNIT_ASSERT(GetValueResult(result.front(), "b")->GetProto().uint64_value() == 0);
        }
        const ui32 reduceStepsCount = 1;
        for (ui32 i = 0; i < reduceStepsCount; ++i) {
            runtime.AdvanceCurrentTime(TDuration::Seconds(numRecords * (i + 1) / reduceStepsCount + 500000));
            const TInstant start = TInstant::Now();
            const ui64 purposeSize = 800000000.0 * (1 - 1.0 * (i + 1) / reduceStepsCount);
            const ui64 purposeRecords = numRecords * (1 - 1.0 * (i + 1) / reduceStepsCount);
            const ui64 purposeMinTimestamp = numRecords * 1.0 * (i + 1) / reduceStepsCount * 1000000;
            while (bsCollector.GetChannelSize(2) > purposeSize && TInstant::Now() - start < TDuration::Seconds(60)) {
                runtime.SimulateSleep(TDuration::Seconds(1));
            }
            Cerr << bsCollector.GetChannelSize(2) << "/" << purposeSize << Endl;

            TVector<THashMap<TString, NYdb::TValue>> result;
            lHelper.StartScanRequest("SELECT MIN(timestamp) as b, COUNT(*) as c FROM `/Root/olapStore/olapTable`", true, &result);
            UNIT_ASSERT(result.size() == 1);
            UNIT_ASSERT(result.front().size() == 2);
            UNIT_ASSERT(GetValueResult(result.front(), "c")->GetProto().uint64_value() == purposeRecords);
            if (purposeRecords) {
                UNIT_ASSERT(GetValueResult(result.front(), "b")->GetProto().uint64_value() == purposeMinTimestamp);
            }

            Y_VERIFY(bsCollector.GetChannelSize(2) <= purposeSize);
        }

        {
            TVector<THashMap<TString, NYdb::TValue>> result;
            lHelper.StartScanRequest("SELECT COUNT(*) FROM `/Root/olapStore/olapTable`", true, &result);
            UNIT_ASSERT(result.front().begin()->second.GetProto().uint64_value() == 0);
        }
    }

}
}
