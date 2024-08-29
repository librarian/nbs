#pragma once
#include "defs.h"
#include <contrib/ydb/library/actors/core/events.h>
#include <contrib/ydb/library/actors/core/event_local.h>
#include <contrib/ydb/library/actors/core/event_pb.h>
#include <contrib/ydb/library/yql/dq/actors/dq_events_ids.h>

#include <contrib/ydb/core/fq/libs/events/event_ids.h>

namespace NKikimr {

struct TKikimrEvents : TEvents {
    enum EEventSpaceKikimr {
        /* WARNING:
           Please mind that you should never change values,
           you should consider issues about "rolling update".
        */
        ES_KIKIMR_ES_BEGIN = ES_USERSPACE, // 4096
        ES_STATESTORAGE = 4097,
        ES_DEPRECATED_4098 = 4098,
        ES_BLOBSTORAGE = 4099,
        ES_HIVE = 4100,
        ES_TABLETBASE = 4101,
        ES_TABLET = 4102,
        ES_TABLETRESOLVER = 4103,
        ES_LOCAL = 4104,
        ES_DEPRECATED_4105 = 4105,
        ES_TX_PROXY = 4106,
        ES_TX_COORDINATOR = 4107,
        ES_TX_MEDIATOR = 4108,
        ES_TX_PROCESSING = 4109,
        ES_DEPRECATED_4110 = 4110,
        ES_DEPRECATED_4111 = 4111,
        ES_DEPRECATED_4112 = 4112,
        ES_TX_DATASHARD = 4113,
        ES_DEPRECATED_4114 = 4114,
        ES_TX_USERPROXY = 4115,
        ES_SCHEME_CACHE = 4116,
        ES_TX_PROXY_REQ = 4117,
        ES_TABLET_PIPE = 4118,
        ES_DEPRECATED_4118 = 4119,
        ES_TABLET_COUNTERS_AGGREGATOR = 4120,
        ES_DEPRECATED_4121 = 4121,
        ES_PROXY_BUS = 4122,
        ES_BOOTSTRAPPER = 4123,
        ES_TX_MEDIATORTIMECAST = 4124,
        ES_DEPRECATED_4125 = 4125,
        ES_DEPRECATED_4126 = 4126,
        ES_DEPRECATED_4127 = 4127,
        ES_DEPRECATED_4128 = 4128,
        ES_DEPRECATED_4129 = 4129,
        ES_DEPRECATED_4130 = 4130,
        ES_DEPRECATED_4131 = 4131,
        ES_KEYVALUE = 4132,
        ES_MSGBUS_TRACER = 4133,
        ES_RTMR_TABLET = 4134,
        ES_FLAT_EXECUTOR = 4135,
        ES_NODE_WHITEBOARD = 4136,
        ES_FLAT_TX_SCHEMESHARD = 4137,
        ES_PQ = 4138,
        ES_YQL_KIKIMR_PROXY = 4139,
        ES_PQ_META_CACHE = 4140,
        ES_DEPRECATED_4141 = 4141,
        ES_PQ_L2_CACHE = 4142,
        ES_TOKEN_BUILDER = 4143,
        ES_TICKET_PARSER = 4144,
        ES_KQP = 4145,
        ES_BLACKBOX_VALIDATOR = 4146,
        ES_SELF_PING = 4147,
        ES_PIPECACHE = 4148,
        ES_PQ_PROXY = 4149,
        ES_CMS = 4150,
        ES_NODE_BROKER = 4151,
        ES_TX_ALLOCATOR = 4152,
        // reserve event space for each RTMR process
        ES_RTMR_STORAGE = 4153,
        ES_RTMR_PROXY = 4154,
        ES_RTMR_PUSHER = 4155,
        ES_RTMR_HOST = 4156,
        ES_RESOURCE_BROKER = 4157,
        ES_VIEWER = 4158,
        ES_SUB_DOMAIN = 4159,
        ES_GRPC_PROXY_STATUS = 4160,
        ES_SQS = 4161,
        ES_BLOCKSTORE = 4162,
        ES_RTMR_ICBUS = 4163,
        ES_TENANT_POOL = 4164,
        ES_USER_REGISTRY = 4165,
        ES_TVM_SETTINGS_UPDATER = 4166,
        ES_PQ_CLUSTERS_UPDATER = 4167,
        ES_TENANT_SLOT_BROKER = 4168,
        ES_GRPC_CALLS = 4169,
        ES_CONSOLE = 4170,
        ES_KESUS_PROXY = 4171,
        ES_KESUS = 4172,
        ES_CONFIGS_DISPATCHER = 4173,
        ES_IAM_SERVICE = 4174,
        ES_FOLDER_SERVICE = 4175,
        ES_GRPC_MON = 4176,
        ES_QUOTA = 4177, // must be in sync with ydb/core/quoter/public/quoter.h
        ES_COORDINATED_QUOTA = 4178,
        ES_ACCESS_SERVICE = 4179,
        ES_USER_ACCOUNT_SERVICE = 4180,
        ES_PQ_PROXY_NEW = 4181,
        ES_GRPC_STREAMING = 4182,
        ES_SCHEME_BOARD = 4183,
        ES_FLAT_TX_SCHEMESHARD_PROTECTED = 4184,
        ES_GRPC_REQUEST_PROXY = 4185,
        ES_EXPORT_SERVICE = 4186,
        ES_TX_ALLOCATOR_CLIENT = 4187,
        ES_PQ_CLUSTER_TRACKER = 4188,
        ES_NET_CLASSIFIER = 4189,
        ES_SYSTEM_VIEW = 4190,
        ES_TENANT_NODE_ENUMERATOR = 4191,
        ES_SERVICE_ACCOUNT_SERVICE = 4192,
        ES_INDEX_BUILD = 4193,
        ES_BLOCKSTORE_PRIVATE = 4194,
        ES_YT_WRAPPER = 4195,
        ES_S3_WRAPPER = 4196,
        ES_FILESTORE = 4197,
        ES_FILESTORE_PRIVATE = 4198,
        ES_YDB_METERING = 4199,
        ES_IMPORT_SERVICE = 4200,
        ES_TX_OLAPSHARD = 4201,
        ES_TX_COLUMNSHARD = 4202,
        ES_CROSSREF = 4203,
        ES_SCHEME_BOARD_MON = 4204,
        ES_YQL_ANALYTICS_PROXY = 4205,
        ES_BLOB_CACHE = 4206,
        ES_LONG_TX_SERVICE = 4207,
        ES_TEST_SHARD = 4208,
        ES_DATASTREAMS_PROXY = 4209,
        ES_IAM_TOKEN_SERVICE = 4210,
        ES_HEALTH_CHECK = 4211,
        ES_DQ = 4212,
        ES_YQ = 4213,
        ES_CHANGE_EXCHANGE_DATASHARD = 4214,
        ES_DATABASE_SERVICE = 4215,
        ES_SEQUENCESHARD = 4216,
        ES_SEQUENCEPROXY = 4217,
        ES_CLOUD_STORAGE = 4218,
        ES_CLOUD_STORAGE_PRIVATE = 4219,
        ES_FOLDER_SERVICE_ADAPTER = 4220,
        ES_PQ_PARTITION_WRITER = 4221,
        ES_YDB_PROXY = 4222,
        ES_REPLICATION_CONTROLLER = 4223,
        ES_HTTP_PROXY = 4224,
        ES_BLOB_DEPOT = 4225,
        ES_DATASHARD_LOAD = 4226,
        ES_METADATA_PROVIDER = 4227,
        ES_INTERNAL_REQUEST = 4228,
        ES_BACKGROUND_TASKS = 4229,
        ES_TIERING = 4230,
        ES_METADATA_INITIALIZER = 4231,
        ES_YDB_AUDIT_LOG = 4232,
        ES_METADATA_MANAGER = 4233,
        ES_METADATA_SECRET = 4234,
        ES_TEST_LOAD = 4235,
        ES_GRPC_CANCELATION = 4236,
        ES_DISCOVERY = 4237,
        ES_EXT_INDEX = 4238,
        ES_CONVEYOR = 4239,
        ES_KQP_SCAN_EXCHANGE = 4240,
        ES_IC_NODE_CACHE = 4241,
        ES_DATA_OPERATIONS = 4242,
        ES_KAFKA = 4243,
        ES_STATISTICS = 4244,
        ES_LDAP_AUTH_PROVIDER = 4245,
        ES_DB_METADATA_CACHE = 4246,
        ES_TABLE_CREATOR = 4247,
        ES_PQ_PARTITION_CHOOSER = 4248,
        ES_GRAPH = 4249,
        ES_REPLICATION_WORKER = 4250,
        ES_CHANGE_EXCHANGE = 4251,
        ES_S3_PROVIDER = 4252,
        ES_NEBIUS_ACCESS_SERVICE = 4253,
        ES_REPLICATION_SERVICE = 4254,
        ES_BACKUP_SERVICE = 4255,
        ES_TX_BACKGROUND = 4256,
        ES_SS_BG_TASKS = 4257,
        ES_LIMITER = 4258,
    };
};

static_assert((int)TKikimrEvents::EEventSpaceKikimr::ES_KQP == (int)NYql::NDq::TDqEvents::ES_DQ_COMPUTE_KQP_COMPATIBLE);
static_assert((int)TKikimrEvents::EEventSpaceKikimr::ES_DQ == (int)NYql::NDq::TDqEvents::ES_DQ_COMPUTE);
static_assert((int)TKikimrEvents::EEventSpaceKikimr::ES_YQL_ANALYTICS_PROXY == (int)NFq::TEventIds::ES_YQL_ANALYTICS_PROXY);

}
