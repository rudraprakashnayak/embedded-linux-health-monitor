#include "config_manager.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace {

void skip_ws(const std::string& s, std::size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
        ++i;
    }
}

bool consume(const std::string& s, std::size_t& i, char c) {
    skip_ws(s, i);
    if (i < s.size() && s[i] == c) {
        ++i;
        return true;
    }
    return false;
}

bool parse_string(const std::string& s, std::size_t& i, std::string& out) {
    skip_ws(s, i);
    if (i >= s.size() || s[i] != '"') {
        return false;
    }
    ++i;
    std::ostringstream oss;
    while (i < s.size()) {
        const char c = s[i++];
        if (c == '"') {
            out = oss.str();
            return true;
        }
        if (c == '\\' && i < s.size()) {
            oss << s[i++];
            continue;
        }
        oss << c;
    }
    return false;
}

bool parse_number(const std::string& s, std::size_t& i, double& out) {
    skip_ws(s, i);
    std::size_t start = i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
        ++i;
    }
    bool any = false;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) {
        any = true;
        ++i;
    }
    if (!any) {
        i = start;
        return false;
    }
    try {
        out = std::stod(s.substr(start, i - start));
        return true;
    } catch (...) {
        return false;
    }
}

bool skip_value(const std::string& s, std::size_t& i);

bool skip_object_or_array(const std::string& s, std::size_t& i, char open_c, char close_c) {
    if (!consume(s, i, open_c)) {
        return false;
    }
    int depth = 1;
    bool in_string = false;
    while (i < s.size() && depth > 0) {
        const char c = s[i++];
        if (in_string) {
            if (c == '\\' && i < s.size()) {
                ++i;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }
        if (c == '"') {
            in_string = true;
        } else if (c == open_c) {
            ++depth;
        } else if (c == close_c) {
            --depth;
        }
    }
    return depth == 0;
}

bool skip_value(const std::string& s, std::size_t& i) {
    skip_ws(s, i);
    if (i >= s.size()) {
        return false;
    }
    if (s[i] == '"') {
        std::string tmp;
        return parse_string(s, i, tmp);
    }
    if (s[i] == '{') {
        return skip_object_or_array(s, i, '{', '}');
    }
    if (s[i] == '[') {
        return skip_object_or_array(s, i, '[', ']');
    }
    double n = 0;
    if (parse_number(s, i, n)) {
        return true;
    }
    if (s.compare(i, 4, "true") == 0) {
        i += 4;
        return true;
    }
    if (s.compare(i, 5, "false") == 0) {
        i += 5;
        return true;
    }
    if (s.compare(i, 4, "null") == 0) {
        i += 4;
        return true;
    }
    return false;
}

bool find_object_key(const std::string& json, const std::string& object, const std::string& key,
                     std::size_t& value_pos) {
    std::size_t i = 0;
    skip_ws(json, i);
    if (!consume(json, i, '{')) {
        return false;
    }

    auto scan_object = [&](std::size_t start) -> bool {
        std::size_t p = start;
        while (p < json.size()) {
            skip_ws(json, p);
            if (p < json.size() && json[p] == '}') {
                return false;
            }
            std::string k;
            if (!parse_string(json, p, k)) {
                return false;
            }
            if (!consume(json, p, ':')) {
                return false;
            }
            if (k == key) {
                skip_ws(json, p);
                value_pos = p;
                return true;
            }
            if (!skip_value(json, p)) {
                return false;
            }
            consume(json, p, ',');
        }
        return false;
    };

    if (object.empty()) {
        return scan_object(i);
    }

    while (i < json.size()) {
        skip_ws(json, i);
        if (i < json.size() && json[i] == '}') {
            return false;
        }
        std::string k;
        if (!parse_string(json, i, k)) {
            return false;
        }
        if (!consume(json, i, ':')) {
            return false;
        }
        if (k == object) {
            skip_ws(json, i);
            if (!consume(json, i, '{')) {
                return false;
            }
            return scan_object(i);
        }
        if (!skip_value(json, i)) {
            return false;
        }
        consume(json, i, ',');
    }
    return false;
}

bool get_number(const std::string& json, const std::string& object, const std::string& key, double& out) {
    std::size_t pos = 0;
    if (!find_object_key(json, object, key, pos)) {
        return false;
    }
    return parse_number(json, pos, out);
}

bool get_int(const std::string& json, const std::string& object, const std::string& key, int& out) {
    double n = 0;
    if (!get_number(json, object, key, n)) {
        return false;
    }
    out = static_cast<int>(n);
    return true;
}

bool get_string(const std::string& json, const std::string& object, const std::string& key, std::string& out) {
    std::size_t pos = 0;
    if (!find_object_key(json, object, key, pos)) {
        return false;
    }
    return parse_string(json, pos, out);
}

}  // namespace

MonitorConfig ConfigManager::defaults() {
    return MonitorConfig{};
}

bool ConfigManager::read_file(const std::string& path, std::string& out, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "Unable to open config file: " + path;
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

bool ConfigManager::parse(const std::string& json, MonitorConfig& out, std::string& error) {
    out = defaults();
    std::size_t i = 0;
    skip_ws(json, i);
    if (i >= json.size() || json[i] != '{') {
        error = "Config root must be a JSON object";
        return false;
    }

    get_int(json, "", "poll_interval_seconds", out.poll_interval_seconds);
    get_int(json, "", "recovery_cooldown_seconds", out.recovery_cooldown_seconds);
    get_string(json, "", "log_file", out.log_file);

    get_number(json, "cpu", "warning_percent", out.cpu.warning_percent);
    get_number(json, "cpu", "critical_percent", out.cpu.critical_percent);
    get_string(json, "cpu", "recovery", out.cpu.recovery);

    get_number(json, "memory", "warning_percent", out.memory.warning_percent);
    get_number(json, "memory", "critical_percent", out.memory.critical_percent);
    get_string(json, "memory", "recovery", out.memory.recovery);

    get_string(json, "disk", "path", out.disk.path);
    get_number(json, "disk", "warning_percent", out.disk.warning_percent);
    get_number(json, "disk", "critical_percent", out.disk.critical_percent);
    get_string(json, "disk", "recovery", out.disk.recovery);

    get_number(json, "temperature", "warning_celsius", out.temperature.warning_celsius);
    get_number(json, "temperature", "critical_celsius", out.temperature.critical_celsius);
    get_string(json, "temperature", "recovery", out.temperature.recovery);

    get_string(json, "network", "interface", out.network.interface_name);
    get_string(json, "network", "check_host", out.network.check_host);
    get_string(json, "network", "recovery", out.network.recovery);

    get_string(json, "service", "name", out.service.name);
    get_string(json, "service", "process", out.service.process);
    get_string(json, "service", "recovery", out.service.recovery);

    if (out.poll_interval_seconds < 1) {
        error = "poll_interval_seconds must be >= 1";
        return false;
    }
    return true;
}

bool ConfigManager::load(const std::string& path, std::string& error) {
    std::string json;
    if (!read_file(path, json, error)) {
        return false;
    }
    return parse(json, config_, error);
}
