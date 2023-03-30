import serial
import threading
import json
import time

# Define ArduinoCommunication class
class ArduinoCommunication:
    def __init__(self, port='COM5', baudrate=115200):
        self.serial = serial.Serial(port, baudrate)
        self.receive_thread = threading.Thread(target=self.receive_message_thread, daemon=True)
        self.stop_receive_thread = False
        self.receive_thread.start()

    def send_message(self, message):
        message_bytes = bytes(message, 'utf-8')
        self.serial.write(message_bytes)

    def receive_message_thread(self):
        while not self.stop_receive_thread:
            if self.serial.in_waiting > 0:
                message = self.serial.readline().decode('utf-8').rstrip()
                try:
                    data = json.loads(message)
                    print('Received message: ', data)
                except json.JSONDecodeError:
                    print('Error decoding message:', message)

    def close(self):
        self.stop_receive_thread = True  # Stop the receive thread
        self.receive_thread.join()
        self.serial.close()

if __name__ == "__main__":
    # Create ArduinoCommunication object
    ser = ArduinoCommunication()

    try:
        while True:
            # Send message to Arduino
            message = '{"command":"reset"}\r\n'
            ser.send_message(message)
            time.sleep(1);

    except KeyboardInterrupt:
        print('Exiting...')
        ser.close()