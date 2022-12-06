#include "commons.h"

using std::string;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::pair;

std::string get_env_var(const std::string& key) {
    char* buff = nullptr;
    size_t sz = 0;

    std::string ret;
    if (_dupenv_s(&buff, &sz, key.c_str()) == 0 && buff != nullptr)
        ret = buff;
    return ret;
}

Traversal readConfigFile(string path) {
    string contents;

    ifstream config(path);
    while (config) {
        contents += config.get();
    }
    config.close();

    vector<pair<string, string>> out;
    int offset = 0;
    while (true) {
        int delim = contents.find("=", offset);
        if (delim == string::npos)
            break;

        int end = contents.find("\n", offset);

        string first = contents.substr(offset, (delim - offset));
        string second = contents.substr(delim + 1, (end - delim - 1));

        out.push_back(pair(first, second));
        offset = end + 1;
    }
    return Traversal(out);
}

void writeConfigFile(vector<pair<string, string>>& list, string path, int write_mode) {
    if (write_mode == configWriter::OVERWRITE)
    {
        ofstream config(path);

        for (auto& p : list) {
            config << p.first << "=" << p.second << std::endl;
        }
        config.close();
    }
    else if (write_mode == configWriter::APPEND) {
        ofstream config(path, std::ios::app);

        for (auto& p : list) {
            config << p.first << "=" << p.second << std::endl;
        }
        config.close();
    }
    else if (write_mode == configWriter::UPDATE) {
        Traversal reader = readConfigFile(path);
        ofstream config(path);
        for (auto& p : list) {
            if (reader[p.first] != "NOT_FOUND")
                reader.set(p.first, p.second);
        }

        for (auto& p : reader.reveal()) {
            config << p.first << "=" << p.second << std::endl;
        }
        config.close();
    }

}


