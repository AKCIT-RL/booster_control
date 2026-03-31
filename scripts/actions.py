#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import sys
import tty
import termios
import select
import os

class ActionSender(Node):
    def __init__(self):
        super().__init__('action_sender')
        self.publisher_ = self.create_publisher(String, '/booster/action', 10)
        
        # Actions based on booster_controller.cpp
        self.actions = {

            # Modes
            '1': 'kPrepare',
            '2': 'kWalking',
            '3': 'kCustom',
            '4': 'kDamping',
            '5': 'kSoccer',

            # End Effector Switch
            'e': 'kSwitchEndEffectorOn',
            'd': 'kSwitchEndEffectorOff',

            #  Commands
            'u': 'kGetUp',
            'l': 'kLieDown',
            's': 'kShoot',
            'x': 'kStopHandEndEffector',
            'O': 'kResetOdometry',
            'W': 'kEnterWBCGait',
            'w': 'kExitWBCGait',

            # Basic Dances
            'n': 'kDanceNewYear',
            'z': 'kDanceNezha',
            'f': 'kDanceTowardsFuture',
            'a': 'kDabbingGesture',
            'U': 'kUltramanGesture', 
            'r': 'kRespectGesture',
            'c': 'kCheeringGesture',
            'L': 'kLuckyCatGesture',
            '.': 'kStopDance',

            # Whole Body Dances / Fight
            'B': 'kBoxingStyleKick',
            'R': 'kRoundhouseKick',
            'M': 'kMoonWalk',
            'm': 'kMichaelDance1',
            
            # Hand Actions
            'h': 'kHandshakeOpen',
            'H': 'kHandshakeClose',
            'v': 'kWaveHandOpen',
            'V': 'kWaveHandClose',

            # Tests
            'T': 'kMoveTest',
            'S': 'kPlaySoundTest',
            'Y': 'kRotateHeadTest',

            # Soccer Moviments
            'b': 'Chapa',
            'g': 'Bica',
            'i': 'kSquatLeft',
            'j': 'kSquatRight',
            'k': 'kSquatUp',
            'l': 'kSquatDown',
            'p': 'kMoveToTargetInternal',
            't': 'kMoveToTargetWithKickTest',
            'E': 'kEnableRobocupWalkMode'

        }
        self.print_menu()

    def print_menu(self):
        print("\n--- Booster Action Sender ---")
        print("PRESS A LETTER/NUMBER KEY matching the command.")
        print("PRESS SPACE to CLEAR screen.")
        print("PRESS 'q' to QUIT.")
        print("----------------------------")

        for key, cmd in sorted(self.actions.items()):
            print(f"  {key}: {cmd}")
            
        print("----------------------------")
        print("Waiting for input...")

    def get_key(self):
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            rlist, _, _ = select.select([sys.stdin], [], [], 0.1)
            if rlist:
                ch = sys.stdin.read(1)
            else:
                ch = None
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

    def send_cmd(self, cmd):
        msg = String()
        msg.data = cmd
        self.publisher_.publish(msg)
        print(f"\n--> Sent: {cmd}")

    def custom_input(self):
        try:
            cmd = input("\nEnter Action String: ")
            if cmd:
                self.send_cmd(cmd)
        except Exception:
            pass

    def run(self):
        try:
            while rclpy.ok():
                key = self.get_key()
                if key == 'q' or key == '\x1b':
                    break
                
                if key == ' ':
                    os.system('clear')
                    self.print_menu()
                elif key in self.actions:
                    self.send_cmd(self.actions[key])
                elif key == '\t':
                    self.custom_input()
                    self.print_menu()
                
                rclpy.spin_once(self, timeout_sec=0.01)
        except KeyboardInterrupt:
            pass
        finally:
            print("\nExiting...")

def main(args=None):
    rclpy.init(args=args)
    node = ActionSender()
    node.run()
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
