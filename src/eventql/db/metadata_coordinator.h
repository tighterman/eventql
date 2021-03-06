/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include "eventql/eventql.h"
#include "eventql/util/status.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/metadata_file.h"
#include "eventql/config/config_directory.h"

namespace eventql {

class MetadataCoordinator {
public:

  MetadataCoordinator(ConfigDirectory* cdir);

  Status performAndCommitOperation(
      const String& ns,
      const String& table_name,
      MetadataOperation op);

  Status performOperation(
      const String& ns,
      const String& table_name,
      MetadataOperation op,
      const Vector<String>& servers);

  Status createFile(
      const String& ns,
      const String& table_name,
      const MetadataFile& file,
      const Vector<String>& servers);

  Status discoverPartition(
      PartitionDiscoveryRequest request,
      PartitionDiscoveryResponse* response);

protected:

  Status createFile(
      const String& ns,
      const String& table_name,
      const MetadataFile& file,
      const String& server);

  Status performOperation(
      const String& ns,
      const String& table_name,
      MetadataOperation op,
      const String& server,
      MetadataOperationResult* result);

  ConfigDirectory* cdir_;
};


} // namespace eventql
