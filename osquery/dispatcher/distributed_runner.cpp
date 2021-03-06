/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed as defined on the LICENSE file found in the
 *  root directory of this source tree.
 */

#include <chrono>

#include <osquery/database.h>
#include <osquery/distributed.h>
#include <osquery/flags.h>
#include <osquery/system.h>

#include <osquery/utils/system/time.h>
#include <osquery/dispatcher/distributed_runner.h>
#include <osquery/utils/conversions/tryto.h>

namespace osquery {

FLAG(uint64,
     distributed_interval,
     60,
     "Seconds between polling for new queries (default 60)")

DECLARE_bool(disable_distributed);
DECLARE_string(distributed_plugin);

const size_t kDistributedAccelerationInterval = 5;

void DistributedRunner::start() {
  auto dist = Distributed();
  while (!interrupted()) {
    dist.pullUpdates();
    if (dist.getPendingQueryCount() > 0) {
      dist.runQueries();
    }

    std::string accelerate_checkins_expire_str = "-1";
    Status status = getDatabaseValue(kPersistentSettings,
                                     "distributed_accelerate_checkins_expire",
                                     accelerate_checkins_expire_str);
    if (!status.ok() || getUnixTime() > tryTo<unsigned long int>(
                                            accelerate_checkins_expire_str, 10)
                                            .takeOr(0ul)) {
      pause(std::chrono::seconds(FLAGS_distributed_interval));
    } else {
      pause(std::chrono::seconds(kDistributedAccelerationInterval));
    }
  }
}

Status startDistributed() {
  if (!FLAGS_disable_distributed) {
    Dispatcher::addService(std::make_shared<DistributedRunner>());
    return Status(0, "OK");
  } else {
    return Status(1, "Distributed query service not enabled.");
  }
}
} // namespace osquery
