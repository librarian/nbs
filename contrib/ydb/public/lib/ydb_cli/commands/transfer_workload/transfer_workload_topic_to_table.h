#pragma once

#include <contrib/ydb/public/lib/ydb_cli/commands/ydb_workload.h>

namespace NYdb::NConsoleClient {

class TCommandWorkloadTransferTopicToTable : public TClientCommandTree {
public:
    TCommandWorkloadTransferTopicToTable();
};

}
