/*
 *  Copyright 2021 The DAPHNE Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <iostream>

#include "WorkerImpl.h"
#include "WorkerImplGRPCAsync.h"
#include "WorkerImplGRPCSync.h"
#include <parser/config/ConfigParser.h>

int main(int argc, char *argv[]) {
    DaphneUserConfig user_config{};
    std::string configFile = "WorkerConfig.json";

    if (argc == 3) {
        configFile = argv[2];
    }

    try {
        if (ConfigParser::fileExists(configFile)) {
            ConfigParser::readUserConfig(configFile, user_config);
        }
    } catch (std::exception &e) {
        spdlog::error("Parser error while reading worker config:\n{}",
                      e.what());
        spdlog::error(
            "You can create a WorkerConfig.json to configure the worker.\n");
    }

    user_config.resolveLibDir();
    auto logger = std::make_unique<DaphneLogger>(user_config);

    if (argc < 2 || argc > 3) {
        std::cout << "Usage: " << argv[0] << " <Address:Port> [ConfigFile]"
                  << std::endl;
        exit(1);
    }
    auto addr = argv[1];

    // TODO choose specific implementation based on arguments or config file
    WorkerImpl *service = new WorkerImplGRPCSync(addr, user_config);

    std::cout << "Started Distributed Worker on `" << addr << "`\n";
    service->Wait();

    return 0;
}