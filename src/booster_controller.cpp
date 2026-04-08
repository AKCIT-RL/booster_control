#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <booster_control/msg/rpc_req_msg.hpp>
#include <booster_control/msg/rpc_resp_msg.hpp>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <functional>
#include <cstdlib>

using nlohmann::json;

class BoosterControllerRaw : public rclcpp::Node
{
public:
    BoosterControllerRaw() : Node("booster_controller_raw"), hand_end_effector_active_(false)
    {
        // Subscribers
        subscription_ = this->create_subscription<std_msgs::msg::String>(
            "/booster/action", 10, std::bind(&BoosterControllerRaw::action_callback, this, std::placeholders::_1));

        resp_subscription_ = this->create_subscription<booster_control::msg::RpcRespMsg>(
            "LocoApiTopicResp", 10, std::bind(&BoosterControllerRaw::resp_callback, this, std::placeholders::_1));

        // Publisher
        publisher_ = this->create_publisher<booster_control::msg::RpcReqMsg>("LocoApiTopicReq", 10);

        init_mappings();
    }

private:
    void init_mappings()
    {

        // kLieDown
        command_map_["kLieDown"] = [this]() {
            RCLCPP_INFO(this->get_logger(), "Executing kLieDown sequence: kPrepare -> Wait 1s -> kLieDown");
            
            // kPrepare
            send_api_req(2000, {{"mode", 1}}); 
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // kLieDown
            send_api_req(2007, json::object());
        };

        command_map_["kGetUp"] = [this]() { send_api_req(2008, json::object()); };
        command_map_["kStopHandEndEffector"] = [this]() { send_api_req(2023, json::object()); };
        command_map_["kShoot"] = [this]() { send_api_req(2024, json::object()); };
        command_map_["kResetOdometry"] = [this]() { send_api_req(2031, json::object()); };
        command_map_["kEnterWBCGait"] = [this]() { send_api_req(2035, json::object()); };
        command_map_["kExitWBCGait"] = [this]() { send_api_req(2036, json::object()); };
        command_map_["kStopSound"] = [this]() { send_api_req(2021, json::object()); };

        // Robot Modes
        auto mode_cmd = [this](int mode_id) {
            return [this, mode_id]() { send_api_req(2000, {{"mode", mode_id}}); };
        };
        command_map_["kDamping"] = mode_cmd(0);
        command_map_["kPrepare"] = mode_cmd(1);
        command_map_["kWalking"] = mode_cmd(2);
        command_map_["kCustom"] = mode_cmd(3);
        command_map_["kSoccer"] = mode_cmd(4);

        // Switch Hand End Effector
        command_map_["kSwitchEndEffectorOn"] = [this]() {
            send_api_req(2012, {{"switch_on", true}});
            hand_end_effector_active_ = true;
        };
        command_map_["kSwitchEndEffectorOff"] = [this]() {
            send_api_req(2012, {{"switch_on", false}});
            hand_end_effector_active_ = false;
        };

        // Dances
        auto add_dance = [this](const std::string& name, int dance_id) {
            command_map_[name] = [this, dance_id, name]() {
                ensure_end_effector_active();
                
                RCLCPP_INFO(this->get_logger(), "Stopping previous dance (1s delay)...");
                send_api_req(2016, {{"dance_id", 1000}}); // kStop = 1000
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                RCLCPP_INFO(this->get_logger(), "Starting dance: %s", name.c_str());
                send_api_req(2016, {{"dance_id", dance_id}});
            };
        };

        add_dance("kDanceNewYear", 0);
        add_dance("kDanceNezha", 1);
        add_dance("kDanceTowardsFuture", 2);
        add_dance("kDabbingGesture", 3);
        add_dance("kUltramanGesture", 4);
        add_dance("kRespectGesture", 5);
        add_dance("kCheeringGesture", 6);
        add_dance("kLuckyCatGesture", 7);
        
        command_map_["kStopDance"] = [this]() { send_api_req(2016, {{"dance_id", 1000}}); };

        // Whole Body Dances
        auto add_wb_dance = [this](const std::string& name, int id) {
            command_map_[name] = [this, id]() {
                send_api_req(2029, {{"dance_id", id}});
            };
        };
        // Mapping matches WholeBodyDanceId enum
        add_wb_dance("kArbicDance", 0);
        add_wb_dance("kMichaelDance1", 1);
        add_wb_dance("kMichaelDance2", 2);
        add_wb_dance("kMichaelDance3", 3);
        add_wb_dance("kMoonWalk", 4);
        add_wb_dance("kBoxingStyleKick", 5);
        add_wb_dance("kRoundhouseKick", 6);

        // Handshake
        command_map_["kHandshakeOpen"] = [this]() { send_api_req(2015, {{"hand_action", 1}}); };
        command_map_["kHandshakeClose"] = [this]() { send_api_req(2015, {{"hand_action", 2}}); };

        // Wave Hand
        command_map_["kWaveHandOpen"] = [this]() { 
            send_api_req(2005, {{"hand_index", 3}, {"hand_action", 1}}); // kRightHand=3, kHandOpen=1
        };
        command_map_["kWaveHandClose"] = [this]() { 
             send_api_req(2005, {{"hand_index", 3}, {"hand_action", 2}}); // kHandClose=2
        };

        // Legacy / Test Commands
        command_map_["kMoveTest"] = [this]() { 
            // kMove
            send_api_req(2001, {{"vx", 0.1}, {"vy", 0.0}, {"vyaw", 0.0}}); 
        };
        command_map_["kRotateHeadTest"] = [this]() { 
            // kRotateHead
            send_api_req(2006, {{"pitch_direction", 1}, {"yaw_direction", 0}}); 
        };
        command_map_["kPlaySoundTest"] = [this]() { 
            // kPlaySound
            send_api_req(2020, {{"sound_file_path", "test.mp3"}}); 
        };
        
        // Internal SDK Commands
        
        // Chapa -> RLKickBall
        command_map_["Chapa"] = [this]() { 
            RCLCPP_INFO(this->get_logger(), "Executing Chapa (RLKickBall)...");
            send_api_req(100011, {{"kick_speed", 1.0}, {"kick_dir", 0.0}});
        }; 
        
        // Bica -> RLFancyKickBall
        command_map_["Bica"] = [this]() { 
            RCLCPP_INFO(this->get_logger(), "Executing Bica (RLFancyKickBall)...");
            send_api_req(100012, {{"kick_speed", 1.0}, {"kick_dir", 0.0}}); 
        };

        // Squats
        command_map_["kSquatLeft"] = [this]() { 
            // kGoalieSquatDown
             send_api_req(100009, {{"squat_direction", -1}, {"squat_side", 0}});
        };
        command_map_["kSquatRight"] = [this]() { 
             // kGoalieSquatDown
             send_api_req(100009, {{"squat_direction", -1}, {"squat_side", 1}});
        };
        command_map_["kSquatDown"] = [this]() { 
            // kSquatAction
            send_api_req(100003, {{"squat_direction", -1}});
        };
        command_map_["kSquatUp"] = [this]() { 
            // kSquatAction
            send_api_req(100003, {{"squat_direction", 1}});
        };

        // MoveToTarget Commands
        command_map_["kMoveToTargetTest"] = [this]() { 
            RCLCPP_INFO(this->get_logger(), "Executing MoveToTarget(1.0, 0.0, 0.0)");

            send_api_req(100000, {
                {"x", 1.0}, {"y", 0.0}, {"yaw", 0.0}, 
                {"is_kick_with_target", false}, 
                {"is_centripetal", false}, 
                {"centripetal_radius", 0.0}
            });
        };

        command_map_["kMoveToTargetWithKickTest"] = [this]() { 
            RCLCPP_INFO(this->get_logger(), "Executing MoveToTargetWithKick(1.0, 0.0, 0.0)");

            send_api_req(100000, {
                {"x", 1.0}, {"y", 0.0}, {"yaw", 0.0}, 
                {"is_kick_with_target", true}, 
                {"is_centripetal", false}, 
                {"centripetal_radius", 0.0}
            });
        };

        command_map_["kEnableRobocupWalkMode"] = [this]() {
            RCLCPP_INFO(this->get_logger(), "Enabling Robocup Walk Mode (100008)");
            send_api_req(100008, json::object());
        };
    }

    void action_callback(const std_msgs::msg::String::SharedPtr msg)
    {
        std::string cmd = msg->data;
        RCLCPP_INFO(this->get_logger(), "Received command: %s", cmd.c_str());

        if (command_map_.count(cmd)) {

            command_map_[cmd]();
        } else {
            RCLCPP_WARN(this->get_logger(), "Unknown command: %s", cmd.c_str());
        }
    }

    void ensure_end_effector_active()
    {
        RCLCPP_INFO(this->get_logger(), "Forcing HandEndEffectorControlMode to TRUE before dance.");
        send_api_req(2012, {{"switch_on", true}});
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        hand_end_effector_active_ = true;
    }

    void resp_callback(const booster_control::msg::RpcRespMsg::SharedPtr msg)
    {
        RCLCPP_INFO(this->get_logger(), "ROBOT RESP << UUID: %s | Header: %s | Body: %s", 
            msg->uuid.c_str(), msg->header.c_str(), msg->body.c_str());
    }

    void send_api_req(int api_id, json body_json)
    {
        auto message = booster_control::msg::RpcReqMsg();
        
        message.uuid = "cmd_" + std::to_string(std::rand());
        
        json header_json;
        header_json["api_id"] = api_id;
        message.header = header_json.dump();
        message.body = body_json.dump();

        publisher_->publish(message);

    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
    rclcpp::Publisher<booster_control::msg::RpcReqMsg>::SharedPtr publisher_;
    rclcpp::Subscription<booster_control::msg::RpcRespMsg>::SharedPtr resp_subscription_;
    
    // State Tracking
    bool hand_end_effector_active_;

    // Command Mapping
    std::map<std::string, std::function<void()>> command_map_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<BoosterControllerRaw>());
    rclcpp::shutdown();
    return 0;
}
