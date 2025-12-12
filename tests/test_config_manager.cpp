#include "../core/config_manager.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

using namespace mp::core;

class ConfigManagerTest : public ::testing::Test {
protected:
    ConfigManager* config_;
    std::string test_config_path_;

    void SetUp() override {
        test_config_path_ = "test_config.json";
        // Clean up any existing test config
        if (std::filesystem::exists(test_config_path_)) {
            std::filesystem::remove(test_config_path_);
        }
        config_ = new ConfigManager(test_config_path_);
        ASSERT_EQ(config_->initialize(), mp::Result::Success);
    }

    void TearDown() override {
        config_->shutdown();
        delete config_;
        // Clean up test file
        if (std::filesystem::exists(test_config_path_)) {
            std::filesystem::remove(test_config_path_);
        }
    }
};

TEST_F(ConfigManagerTest, SetAndGetString) {
    config_->set_string("audio", "device", "test_device");
    EXPECT_EQ(config_->get_string("audio", "device"), "test_device");
}

TEST_F(ConfigManagerTest, SetAndGetInt) {
    config_->set_int("audio", "sample_rate", 48000);
    EXPECT_EQ(config_->get_int("audio", "sample_rate"), 48000);
}

TEST_F(ConfigManagerTest, SetAndGetBool) {
    config_->set_bool("playback", "gapless", true);
    EXPECT_TRUE(config_->get_bool("playback", "gapless"));
}

TEST_F(ConfigManagerTest, SetAndGetFloat) {
    config_->set_float("playback", "volume", 0.75f);
    EXPECT_FLOAT_EQ(config_->get_float("playback", "volume"), 0.75f);
}

TEST_F(ConfigManagerTest, DefaultValues) {
    EXPECT_EQ(config_->get_string("nonexistent", "key", "default"), "default");
    EXPECT_EQ(config_->get_int("nonexistent", "key", 42), 42);
    EXPECT_TRUE(config_->get_bool("nonexistent", "key", true));
    EXPECT_FLOAT_EQ(config_->get_float("nonexistent", "key", 0.5f), 0.5f);
}

TEST_F(ConfigManagerTest, SaveAndLoad) {
    // Set values
    config_->set_string("audio", "device", "saved_device");
    config_->set_int("audio", "buffer_size", 2048);
    
    // Save
    config_->save();
    
    // Verify file exists
    EXPECT_TRUE(std::filesystem::exists(test_config_path_));
    
    // Create new config manager and load
    ConfigManager config2(test_config_path_);
    ASSERT_EQ(config2.initialize(), mp::Result::Success);
    
    // Verify loaded values
    EXPECT_EQ(config2.get_string("audio", "device"), "saved_device");
    EXPECT_EQ(config2.get_int("audio", "buffer_size"), 2048);
    
    config2.shutdown();
}

TEST_F(ConfigManagerTest, AutoSave) {
    config_->set_auto_save(true);
    config_->set_string("test", "value", "auto_saved");
    
    // Shutdown should trigger auto-save
    config_->shutdown();
    
    // Load in new instance
    ConfigManager config2(test_config_path_);
    ASSERT_EQ(config2.initialize(), mp::Result::Success);
    EXPECT_EQ(config2.get_string("test", "value"), "auto_saved");
    config2.shutdown();
}

TEST_F(ConfigManagerTest, ChangeNotification) {
    bool callback_invoked = false;
    std::string notified_section;
    std::string notified_key;
    
    auto handle = config_->register_change_listener(
        [&](const std::string& section, const std::string& key) {
            callback_invoked = true;
            notified_section = section;
            notified_key = key;
        }
    );
    
    config_->set_string("audio", "device", "new_device");
    
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(notified_section, "audio");
    EXPECT_EQ(notified_key, "device");
    
    config_->unregister_change_listener(handle);
}

TEST_F(ConfigManagerTest, MultipleListeners) {
    int callback_count = 0;
    
    auto handle1 = config_->register_change_listener(
        [&](const std::string&, const std::string&) { callback_count++; }
    );
    
    auto handle2 = config_->register_change_listener(
        [&](const std::string&, const std::string&) { callback_count++; }
    );
    
    config_->set_string("test", "key", "value");
    EXPECT_EQ(callback_count, 2);
    
    config_->unregister_change_listener(handle1);
    config_->unregister_change_listener(handle2);
}

TEST_F(ConfigManagerTest, SchemaVersioning) {
    // Save with current schema version
    config_->set_string("test", "key", "value");
    config_->save();
    
    // Read raw JSON and check for schema version
    std::ifstream file(test_config_path_);
    ASSERT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_NE(content.find("_schema_version"), std::string::npos);
}
