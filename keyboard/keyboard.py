
#Send & receive messages to control a drone
import olympe

#Ability to use sleep for instance
import time

#Ability to execute the gcc command
import subprocess

#PCMD moves the drone, it sends commands every 50ms
from olympe.messages.ardrone3.Piloting import TakeOff, Landing, PCMD

#Used to start the mission automatically
from olympe.messages import mission

#Ability to use the keyboard : Key is for special keys, KeyCode get the character value
from pynput.keyboard import Listener, Key, KeyCode

#Never raises a KeyError, it provides a default value for the key that does not work
from collections import defaultdict

#Ability to define a set of named constants
from enum import Enum 

#Enum of the different types of moves the drone can perform
class Ctrl(Enum):
    (
        QUIT,
        TAKEOFF,
        LANDING,
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_FORWARD,
        MOVE_BACKWARD,
        MOVE_UP,
        MOVE_DOWN,
        TURN_LEFT,
        TURN_RIGHT,
    ) = range(11)

#Dictionary to store key values & what moves they match with
AZERTY_CTRL_KEYS = {
    Ctrl.QUIT: Key.esc,
    Ctrl.TAKEOFF: "t",
    Ctrl.LANDING: "l",
    Ctrl.MOVE_LEFT: "q",
    Ctrl.MOVE_RIGHT: "d",
    Ctrl.MOVE_FORWARD: "z",
    Ctrl.MOVE_BACKWARD: "s",
    Ctrl.MOVE_UP: Key.up,
    Ctrl.MOVE_DOWN: Key.down,
    Ctrl.TURN_LEFT: Key.left,
    Ctrl.TURN_RIGHT: Key.right,
}

def run_qr_scanner(image_path):
    compile_command = "gcc -o qrscanner main.c QRCodeScanner.c -Ideps/zbar/zbar-0.10 -lzbar -ljpeg -lm"
    subprocess.run(compile_command, shell=True, check=True)

    scan_command = f"./qrscanner {image_path}"
    result = subprocess.run(scan_command, shell=True, capture_output=True, text=True)

    print("QR Scanner Output:", result.stdout)
    print("QR Scanner Error (if any):", result.stderr)

#Listener allows to capture keyboard events
class KeyboardCtrl(Listener):
    def __init__(self, ctrl_keys=None): #None is when the key is unknown
        self._ctrl_keys = self._get_ctrl_keys(ctrl_keys)    #Call the method to get the stored keys
        self._key_pressed = defaultdict(lambda: False)  #Create a dictionary where if the key hasn't been pressed, it returns False
        self._last_action_ts = defaultdict(lambda: 0.0) #Create a dictionary where if the key hasn't been recorded yet, the value is 0.0
        super().__init__(on_press=self._on_press, on_release=self._on_release)
        self.start()    #Starts the listening process

    #Monitor key press events
    def _on_press(self, key):
        if isinstance(key, KeyCode):    #Checks if the key pressed is a known key
            self._key_pressed[key.char] = True  #The key is considered pressed and its state is True and stored in key_pressed
        elif isinstance(key, Key):  #Check if the key pressed is a special key
            self._key_pressed[key] = True   #The key is considered pressed and its state is True and stored in key_pressed
        if self._key_pressed[self._ctrl_keys[Ctrl.QUIT]]:   #Check if the key pressed is esc
            return False    #Return False which stops the listener loop
        else:
            return True #If nothing is detected, True is sent and the loop keeps going

    #Monitor key release events, it resets each key's status when released
    def _on_release(self, key):
        if isinstance(key, KeyCode):
            self._key_pressed[key.char] = False
        elif isinstance(key, Key):
            self._key_pressed[key] = False
        return True #Keep the listener active

    #Stopping the listener & quit
    def quit(self):
        return not self.running or self._key_pressed[self._ctrl_keys[Ctrl.QUIT]]

    #The returned value is negative when turning left and positive when turning right
    def _axis(self, left_key, right_key):
        return 100 * (   #multiplying by 100 to increase the range of values and have smoother drone movements, cf doc Olympe
            int(self._key_pressed[right_key]) - int(self._key_pressed[left_key])    #int converts True to 1 and False to 0
        )


    #Roll the drone to the right if the result is -100 and 100 if it is to the left. 0 if both keys are pressed or none
    def roll(self):
        return self._axis(
            self._ctrl_keys[Ctrl.MOVE_LEFT],
            self._ctrl_keys[Ctrl.MOVE_RIGHT]
        )

    #Moves the drone to the front if the result is -100 and 100 if it is backwards. 0 if both keys are pressed or none
    def pitch(self):
        return self._axis(
            self._ctrl_keys[Ctrl.MOVE_BACKWARD],
            self._ctrl_keys[Ctrl.MOVE_FORWARD]
        )

    #Rotationate the drone to the left if the result is 100 and -100 if it is to the right. 0 if both keys are pressed or none
    def yaw(self):
        return self._axis(
            self._ctrl_keys[Ctrl.TURN_LEFT],
            self._ctrl_keys[Ctrl.TURN_RIGHT]
        )

    #Moves the drone upwards if the result is -100 and 100 if it is downwards. 0 if both keys are pressed or none
    def throttle(self):
        return self._axis(
            self._ctrl_keys[Ctrl.MOVE_DOWN],
            self._ctrl_keys[Ctrl.MOVE_UP]
        )

    #Checks if any of the precedent methods have been called. If called (methods return -100 or 100), then True is returned. If not called (0), then false is returned
    def has_piloting_cmd(self):
        return (
            bool(self.roll())
            or bool(self.pitch())
            or bool(self.yaw())
            or bool(self.throttle())
        )

    #Assure that the drone does not receive too many commands at once
    def _rate_limit_cmd(self, ctrl, delay):
        now = time.time()   #Retrieves the current time
        if self._last_action_ts[ctrl] > (now - delay):  #Was the last action issued within the delay period
            return False    #The command should not be executed for now
        elif self._key_pressed[self._ctrl_keys[ctrl]]:  #Is the corresponding key for the command currently pressed
            self._last_action_ts[ctrl] = now    #Updates the timestamp for the last action to now
            return True #The command can be executed
        else:
            return False    #If neither conditions is met, it returns False


    #Calls the time_limit_fct to either wait or execute the command (the delay is 2s)
    def takeoff(self):
        return self._rate_limit_cmd(Ctrl.TAKEOFF, 2.0)

    #Calls the time_limit_fct to either wait or execute the command (the delay is 2s)
    def landing(self):
        return self._rate_limit_cmd(Ctrl.LANDING, 2.0)

    #Get the keys stored in CONTROL_KEYS :
    def _get_ctrl_keys(self, ctrl_keys):
        if ctrl_keys is None:
            ctrl_keys = AZERTY_CTRL_KEYS

        return ctrl_keys


if __name__ == "__main__":
    with olympe.Drone("10.202.0.1") as drone:   #with ensures that the drone is connected and will be disconnected when blocl exited
        drone.connect()

        run_qr_scanner("qrcode.jpeg")

        control = KeyboardCtrl()
        while not control.quit():   #While key esc is not pressed
            if control.takeoff():
                drone(TakeOff())
            elif control.landing():
                drone(Landing())
            if control.has_piloting_cmd():
                drone(
                    PCMD(
                        1,
                        control.roll(),
                        control.pitch(),
                        control.yaw(),
                        control.throttle(),
                        timestampAndSeqNum=0,   #Signals that juste simples commands are sent
                    )
                )
            else:
                drone(PCMD(0, 0, 0, 0, 0, timestampAndSeqNum=0))    #If nothing is send, this command is sent to stop any movement. timestampAndSeqNum allows to process commands in the correct sequence, it is 0 because it has no active control commands
            time.sleep(0.05)    #delay of 50ms to control the frequency of command updates
