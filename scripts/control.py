#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from booster_control.msg import RemoteControllerState
from std_msgs.msg import String
import sys
import termios
import tty
import select
import time

class F710Simulator(Node):
    def __init__(self):
        super().__init__('f710_simulator')
        self.state_publisher_ = self.create_publisher(RemoteControllerState, '/remote_controller_state', 10)
        self.action_publisher_ = self.create_publisher(String, '/booster/action', 10)
        self.print_menu()

    def print_menu(self):
        print("\n--- Logitech F710 Simulator ---")
        print("Select a combination to simulate (press number):")
        print("1: LT + RT + A")
        print("2: LT + A")
        print("3: LT + B")
        print("4: RT + A")
        print("q: Quit")
        print("-------------------------------")

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

    def create_empty_msg(self):
        msg = RemoteControllerState()
        return msg

    def publish_input(self, setup_func):
        
        # Creating message
        msg = self.create_empty_msg()
        setup_func(msg)
        
        print(f"\nSending input...")
        for _ in range(10):
            self.state_publisher_.publish(msg)
            time.sleep(0.01)
        
        empty_msg = self.create_empty_msg()
        self.state_publisher_.publish(empty_msg)
        print("Done.")

    def run(self):
        try:
            while rclpy.ok():
                key = self.get_key()
                if key == 'q' or key == '\x1b':
                    break
                
                if key == '1':   # LT + RT + A
                    self.publish_input(lambda m: setattr(m, 'lt', True) or setattr(m, 'rt', True) or setattr(m, 'a', True))
                elif key == '2': # LT + A
                    self.publish_input(lambda m: setattr(m, 'lt', True) or setattr(m, 'a', True))
                    self.action_publisher_.publish(String(data='kEnableRobocupWalkMode'))
                elif key == '3': # LT + B
                    self.publish_input(lambda m: setattr(m, 'lt', True) or setattr(m, 'b', True))
                elif key == '4': # RT + A
                    self.publish_input(lambda m: setattr(m, 'rt', True) or setattr(m, 'a', True))
                
                if key in ['1', '2', '3', '4']:
                    self.print_menu()
                
                rclpy.spin_once(self, timeout_sec=0.01)
        except KeyboardInterrupt:
            pass
        finally:
            print("\nExiting...")

def main(args=None):
    rclpy.init(args=args)
    node = F710Simulator()
    node.run()
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
