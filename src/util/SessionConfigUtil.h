//  SessionConfigUtil.h
//  ofxMarkSynth
//
//  Utility for choosing and persisting a "session config" JSON file path.
//

#pragma once

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>

#include "ofMain.h"

namespace ofxMarkSynth {

struct SessionConfigSelectorOptions {
    std::string appNamespace;
    std::string dialogTitle = "Choose session config (JSON)";
    int forceChooseKey = OF_KEY_SHIFT;
    std::filesystem::path chooserDefaultDirectory = std::filesystem::path(ofFilePath::getUserHomeDir()) / "Documents";
    std::string pointerFilename = "lastSessionConfig.json";
};

inline std::filesystem::path getSessionConfigPointerFilePath(const std::string& appNamespace, const std::string& pointerFilename) {
    auto appSupportDir = std::filesystem::path(ofFilePath::getUserHomeDir())
        / "Library" / "Application Support" / appNamespace;
    return appSupportDir / pointerFilename;
}

inline std::optional<std::filesystem::path> loadLastSessionConfigPath(const std::filesystem::path& pointerFilePath) {
    if (!std::filesystem::exists(pointerFilePath)) {
        return std::nullopt;
    }

    const ofJson json = ofLoadJson(pointerFilePath);
    if (json.empty()) {
        return std::nullopt;
    }

    if (!json.contains("configPath") || !json["configPath"].is_string()) {
        ofLogWarning("SessionConfigUtil") << "Invalid pointer file: missing string 'configPath' in " << pointerFilePath;
        return std::nullopt;
    }

    std::filesystem::path configPath { json["configPath"].get<std::string>() };
    if (!std::filesystem::exists(configPath)) {
        ofLogWarning("SessionConfigUtil") << "Saved config path does not exist: " << configPath;
        return std::nullopt;
    }

    return configPath;
}

inline bool saveLastSessionConfigPath(const std::filesystem::path& pointerFilePath, const std::filesystem::path& configPath) {
    std::error_code ec;
    std::filesystem::create_directories(pointerFilePath.parent_path(), ec);
    if (ec) {
        ofLogWarning("SessionConfigUtil") << "Failed to create Application Support dir: "
                                          << pointerFilePath.parent_path() << " (" << ec.message() << ")";
        return false;
    }

    ofJson json;
    json["configPath"] = configPath.string();
    return ofSavePrettyJson(pointerFilePath, json);
}

inline std::optional<std::filesystem::path> chooseSessionConfigPathWithDialog(const SessionConfigSelectorOptions& options) {
    auto result = ofSystemLoadDialog(options.dialogTitle, false, options.chooserDefaultDirectory.string());
    if (!result.bSuccess) {
        return std::nullopt;
    }

    return std::filesystem::path { result.getPath() };
}

[[noreturn]] inline void hardExitWithAlert(const std::string& message, int status = 1) {
    ofLogError("SessionConfigUtil") << message;
    ofSystemAlertDialog(message);

    // Request a normal oF shutdown, but also hard-exit.
    // This ensures we exit cleanly even if we're still in setup.
    ofExit(status);
    std::exit(status);
}

inline std::optional<std::filesystem::path> resolveSessionConfigPath(const SessionConfigSelectorOptions& options) {
    if (options.appNamespace.empty()) {
        ofLogError("SessionConfigUtil") << "appNamespace must not be empty";
        return std::nullopt;
    }

    const auto pointerFilePath = getSessionConfigPointerFilePath(options.appNamespace, options.pointerFilename);

    const bool forceChoose = ofGetKeyPressed(options.forceChooseKey);
    std::optional<std::filesystem::path> configPath;
    if (!forceChoose) {
        configPath = loadLastSessionConfigPath(pointerFilePath);
    }

    if (!configPath) {
        configPath = chooseSessionConfigPathWithDialog(options);
        if (configPath) {
            saveLastSessionConfigPath(pointerFilePath, *configPath);
        }
    }

    if (!configPath) {
        return std::nullopt;
    }

    if (configPath->extension() != ".json") {
        ofLogWarning("SessionConfigUtil") << "Config file must be a .json: " << *configPath;
        return std::nullopt;
    }

    return configPath;
}

struct SessionConfig {
    std::filesystem::path path;
    ofJson json;
};

inline SessionConfig loadSessionConfigOrExit(const SessionConfigSelectorOptions& options) {
    auto configPathOpt = resolveSessionConfigPath(options);
    if (!configPathOpt) {
        hardExitWithAlert("No session config selected. Exiting.");
    }

    const auto& configPath = *configPathOpt;
    ofLogNotice(options.appNamespace) << "Using session config: " << configPath;

    const ofJson json = ofLoadJson(configPath);
    if (json.empty()) {
        hardExitWithAlert("Failed to load session config JSON. Exiting.");
    }

    return { configPath, json };
}

} // namespace ofxMarkSynth
