#pragma once

#include <fstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include <utility>
#include "include/Traversal.h"

std::string get_env_var(const std::string& key);

namespace configWriter {
    const int UPDATE = 0;
    const int OVERWRITE = 1;
    const int APPEND = 2;

    const std::string DEFAULT_PATH = get_env_var("APPDATA") + "\\SyncUp\\SU_Config.conf";

}

namespace paths {
    const std::string TMP = get_env_var("TMP");
    const std::string USERPROFILE = get_env_var("USERPROFILE");
    const std::string APPDATA = get_env_var("APPDATA");
    const std::string SYNCUP_DATA_DIR = APPDATA + "/SyncUp/";
    const std::string SYNCUP_INSTALL_DIR = get_env_var("PROGRAMFILES") + "/SyncUp/";
}

Traversal readConfigFile(std::string path);

void writeConfigFile(std::vector<std::pair<std::string, std::string>>& list, std::string path = configWriter::DEFAULT_PATH, int write_mode = configWriter::UPDATE);

