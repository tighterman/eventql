/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/tasks/ReduceTask.h"
#include "zbase/mapreduce/MapReduceScheduler.h"

using namespace stx;

namespace zbase {

ReduceTask::ReduceTask(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job_spec,
    const String& method_name,
    Vector<RefPtr<MapReduceTask>> sources,
    size_t num_shards,
    AnalyticsAuth* auth,
    zbase::ReplicationScheme* repl) :
    session_(session),
    job_spec_(job_spec),
    method_name_(method_name),
    sources_(sources),
    num_shards_(num_shards),
    auth_(auth),
    repl_(repl) {}

Vector<size_t> ReduceTask::build(MapReduceShardList* shards) {
  Vector<size_t> out_indexes;
  Vector<size_t> in_indexes;

  for (const auto& src : sources_) {
    auto src_indexes = src->build(shards);
    in_indexes.insert(in_indexes.end(), src_indexes.begin(), src_indexes.end());
  }

  for (size_t shard_idx = 0; shard_idx < num_shards_; shard_idx++) {
    auto shard = mkRef(new MapReduceTaskShard());
    shard->task = this;
    shard->dependencies = in_indexes;

    out_indexes.emplace_back(shards->size());
    shards->emplace_back(shard);
  }

  return out_indexes;
}

Option<MapReduceShardResult> ReduceTask::execute(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job) {
  Vector<String> input_tables;
  for (const auto& input : shard->dependencies) {
    auto input_tbl = job->getResultURL(input);
    if (input_tbl.isEmpty()) {
      continue;
    }

    input_tables.emplace_back(input_tbl.get());
  }

  auto output_id = Random::singleton()->sha1(); // FIXME

  Vector<String> errors;
  auto hosts = repl_->replicasFor(output_id);
  for (const auto& host : hosts) {
    try {
      return executeRemote(shard, job, input_tables, host);
    } catch (const StandardException& e) {
      logError(
          "z1.mapreduce",
          e,
          "MapTableTask::execute failed");

      errors.emplace_back(e.what());
    }
  }

  RAISEF(
      kRuntimeError,
      "MapTableTask::execute failed: $0",
      StringUtil::join(errors, ", "));
}

Option<MapReduceShardResult> ReduceTask::executeRemote(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job,
    const Vector<String>& input_tables,
    const ReplicaRef& host) {
  logDebug(
      "z1.mapreduce",
      "Executing remote reduce shard on $2; customer=$0 input_tables=$1",
      session_.customer(),
      input_tables.size(),
      host.addr.hostAndPort());

  auto url = StringUtil::format(
      "http://$0/api/v1/mapreduce/tasks/reduce?" \
      "program_source=$1&method_name=$2",
      host.addr.ipAndPort(),
      URI::urlEncode(job_spec_->program_source),
      URI::urlEncode(method_name_));

  for (const auto& input_table : input_tables) {
    url += "&input_table=" + URI::urlEncode(input_table);
  }

  auto api_token = auth_->encodeAuthToken(session_);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  http::HTTPClient http_client;
  auto req = http::HTTPRequest::mkGet(url, auth_headers);
  auto res = http_client.executeRequest(req);

  if (res.statusCode() == 204) {
    return None<MapReduceShardResult>();
  }

  if (res.statusCode() != 201) {
    RAISEF(
        kRuntimeError,
        "received non-201 response: $0", res.body().toString());
  }

  MapReduceShardResult result {
    .host = host,
    .result_id = SHA1Hash::fromHexString(res.body().toString())
  };

  return Some(result);
}

} // namespace zbase
